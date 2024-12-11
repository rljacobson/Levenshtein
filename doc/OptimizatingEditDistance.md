# Common Optimizations For Computing Levenshtein Edit Distance
---
title: 'Common Optimizations for Levenshtein Edit Distance'
date: 2024-12-02T21:18:14-06:00
draft: False
featured_image: "imgs/posts/Levenshtein.jpg"
tags: [Algorithms, Computer Science, Software Engineering]
math: False
author: "Robert Jacobson"
---

I have been studying algorithms to compute the [Levenshtein edit distance](https://en.wikipedia.org/wiki/Levenshtein_distance) between two strings, which is defined as the minimum number of "edits" required to transform one string into another string. This is useful for a variety of things, from DNA sequence alignment to spell correction to address validation. I am studying it to resolve misspelled and OCR'ed scientific names of organisms to their real names, a famous problem in the field. The idea, of course, is to search a list of correct names for the given name and select the correct name that has the lowest distance to the given name. Sometimes this is called fuzzy matching.

There are lots of other great articles you can read to learn about this metric and about the standard algorithm to compute it, so I won't go into those details here. Read one of those articles and then come back here once you have a vague idea about how it works.

Right, ok, the main idea is that we construct a matrix in which a cell at `(row, col) = (i, j)` stores the minimum number of edits required to transform the first `i` characters of the first string into the first `j` characters of the second string, and this is done by taking the smallest number of edits from among the choices of `delete`, `insert`, and `substitute`. The number at position `(m, n)`, where `m` and `n` are the lengths of the strings, is the final edit distance. If you keep track of _which_ edits result in the minimum value at each cell (there may be more than one) while you compute the matrix, you can even recover the sequence(s) of edits required to do the job by tracing backwards from the lower right corner to the upper left corner. This is sometimes called the Wagner–Fischer algorithm. Here's the obligatory cute example:

```
Standard Algorithm (max_cost = 4)
       d   o   g   b   e   r   t
   0 → 1 → 2 → 3 → 4 → 5 → 6 → 7
   ↓ ↘   ↘   ↘   ↘   ↘   ⇘
r  1   1 → 2 → 3 → 4 → 5   5 → 6
   ↓ ↘ ↓ ⇘                   ↘
o  2   2   1 → 2 → 3 → 4 → 5 → 6
   ↓ ↘ ↓   ↓ ↘   ⇘
b  3   3   2   2   2 → 3 → 4 → 5
   ↓ ↘ ↓ ⇘ ↓ ↘ ↓ ↘ ↓ ↘   ↘   ↘
o  4   4   3   3   3   3 → 4 → 5
   ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓ ↘ ↓ ↘   ⇘
t  5   5   4   4   4   4   4   4
```

If you are fuzzy searching a very large database, or if you are doing a lot of searches, or if your strings are extremely long, then it is important to do this computation very fast. There are specialized algorithms for a variety of special situations, some using GPUs or SIMD instructions, some in which the list being searched is preprocessed, and so forth. I will discuss only the pedestrian case here, though the others are also fascinating.

## Major Optimizations

The most significant optimizations that you can make, at least for the case that you compare a single string against a large number of other strings, have very little to do with the Levenshtein distance computation itself. They are the first two we tackle.

### Remember The Closest Match

In the special _but very common_ case that you are trying to find the closest string in a large collection of strings to a single particular string, the best optimization you can do is to remember the smallest distance you have seen so far and use that distance as the `max_edits` parameter in any limited distance / banded algorithm variant (discussed below). The speedup is significant. For both synthetic and real-world data I have found that this algorithm takes only 2/3rds as long to run as the other algorithms. Your mileage may vary, of course.

### Allocate Once

Many real-world implementations, including those implementations intended for use in industrial strength databases, allocate memory on ever single call. Allocation is a slow operation, so this is quite wasteful. In MySQL/MariaDB, for example, UDFs provide an initialization function and a mechanism to remember data between function calls:

```c++
int edit_dist_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // ...other code to check the number and type of arguments or anything else...

    // Preallocate a buffer. A pointer to it can be saved in `UDF_INIT::ptr`.
    initid->ptr = (char *)new(std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    if (initid->ptr == nullptr) {
        return 1;
    }

    // ... any other necessary code...
}

void edit_dist_deinit(UDF_INIT *initid) {
    // Don't forget to properly release the memory!
    delete[] reinterpret_cast<int*>(initid->ptr);
}

long long lev(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {

    // Fetch the preallocated buffer.
    int *buffer = reinterpret_cast<int *>(initid->ptr);

    // ...the rest of the algorithm...
}
```

In cases where it's feasible, such as database queries, allocate memory only a single time and reuse that memory on
every function call. As always, be sure to reclaim that memory on teardown.

### Trimming

Under some circumstances, trimming any common prefix or suffix prior to running the algorithm can be advantageous. The resulting strings are shorter, and in the max-limited case the length differences between the strings might already exceed the max allowed distance, letting you return without doing the algorithm at all. This is especially useful when

1. the strings are long, and
2. the strings are known to be similar, and
3. you can use SIMD or other specialized instructions to perform the trimming very quickly.

Under these conditions my experiments show a 3x speedup in the typical case. However, for the use case of searching a haystack for a needle, the vast majority of string pairs will not have a common prefix or suffix, so SIMD trimming in particular will be a lot slower than not trimming at all.

For non-SIMD trimming, for reasonably short strings (<250 characters), if the other optimizations given here are implemented, trimming won't give any measurable advantage even when the strings are identical--at least it doesn't on my machine. On the other hand, this means that trimming is essentially "free" to do--it's a wash--so if you have some other external reason to trim the strings, you might as well.


### Limited Distance Variant (The Banded Algorithm)

In many situations, you only care about computing distances that are "small," say, less than some number `max_edits`. Now, edit distance is _always_ bounded above by the length of the longest string, so we'll assume `max_edits` is always smaller than that. In such a case we can use a "banded" variant of the standard algorithm. The basic idea is, we don't have to compute the cells of the matrix for which we can prove that the edit distance from that cell will exceed `max_edits`, and if you can prove that the distance will exceed `max_edits` no matter what, you don't have to finish the computation at all. Just return `infinity` or `max_edits + 1` or whatever makes sense to you.

#### Bailing Early

Observe that if one string is longer than the other, then you will need to insert characters into the shorter one (equivalently, delete characters from the longer one) to make the strings the same length. Assuming `m > n`, you will need at least `m-n` insertions. So if `m-n > max_edits`, you're done! And it only costs one subtraction and one comparison. This exit condition can be combined with any of the other optimizations discussed here.

Another simple idea is, while computing each row, to keep track of the minimum distance computed in the row, and if that minimum row distance ever exceeds `max_edits`, you know that the final edit distance will exceed `max_edits`, because the minimum edit distance is nondecreasing with each row. (This optimization is not needed if you use optimal band narrowing discussed below, but other banded algorithms can benefit.)

These two optimizations are quite common in real-world implementations.

#### Narrowing the Band

##### Constant Width Band

The simplest way to construct the band is to observe that if we do more than `max_edits` inserts or deletions, then we
will obviously exceed `max_edits`. Therefore, we only need to compute `matrix(i, j)` where the column `j` satisfies `i -
max_edits <= j <= i + max_edits`. So the band has a width of `2*max_edits + 1`, and you end up computing a total of
`(2*max_edits + 1)*n` cells, where here we assume WLOG that `m>=n`.  (The `+1` is for the cell exactly on the diagonal itself.) This is the most common banded variant I have seen
in the wild.

We can do better by noticing that `(m-n)` insertions are required in order to make the strings the same length. The quantity `max_edits - (m-n)` represents the remaining cost budget after accounting for the `(m-n)` insertions we know are required. It is possible for there to be additional insertions beyond the required `(m-n)` insertions, but for every additional insertion beyond the `(m-n)`, there will have to be a corresponding deletion for the strings to end up the same length. Therefore, for every additional insertion, the total cost will be 2, one for the insertion and one for the corresponding deletion. There can be at most `(max_edits -(m-n))/2` such insertions, because `2*(max_edits - (m-n))/2 = max_edits - (m-n)`, our remaining cost budget which we are not allowed to exceed. The upshot is that now our band can only go a distance `max_edits - (max_edits -(m-n))/2 = (max_edits + m-n) / 2 ` away from the diagonal. Therefore, the band has total width `2*(max_edits + m-n) / 2 + 1 = max_edits + m-n + 1`. Since we already assume that `m - n < max_edits` (because otherwise we would have bailed early), this band width is _strictly better_ than the one in the previous paragraph.

I rarely see this optimization employed in implementations with constant band width even though it is mentioned in the literature. My guess is it's because academics are terrible communicators and so it can be difficult for non academics to understand why the formula works.

##### Dynamic Width Band

There are a few different ways to narrow the band dynamically. A simple way is to just subtract the minimum edit distance within a row from your total cost budget. On my own machine, this counterintuitively performs _worse_ than having a constant band width. The most likely reason is that the benefits of computing fewer cells are outweighed by the increase in [branch mispredictions](https://en.wikipedia.org/wiki/Predication_(computer_architecture)). The lesson here is that dynamically adjusting the band width has to give a big enough advantage that it pays for itself. Of course, you should benchmark and measure _on your own machine_.

There are a few other heuristic methods of narrowing the band, but let's skip to the optimal band width. First we will establish some terminology, and then we will derive the stop condition.

The diagonal cells of the matrix *starting from the upper left corner* represent when the strings are the same length. The diagonal cells of the matrix *starting from the lower right corner* represent when the strings have the same number of characters remaining. You can think of this second diagonal as where you are after spending the `(m-n)` inserts into the shorter string to obtain the longer string (or equivalently, the deletions from the longer string to obtain the shorter string). We shall call it the *right diagonal*. On the `i`th row the right diagonal is at column `i + (m-n)`. Also, we will call the first column in the band `start_j` and the last column in the band `end_j`, so that we compute all cells `matrix(i, j)` for `start_j <= j <= end_j` (inclusive). Recall that ultimately the final edit distance will be computed in the bottom right corner, `matrix(m, n)`.

After computing a row, we have additional information about our remaining cost budget. Assume for the moment that
`end_j` is large enough that it extends _beyond_ the right diagonal. The cell `matrix(i, end_j)` represents both the
insertions needed to reach the right diagonal, the additional insertions to reach `end_j`, and any additional edits that
may have occurred. But recall that every insertion beyond the right diagonal (beyond `(m-n)` insertions) must
necessarily be paired with a deletion. The number of additional insertions at `end_j` is `|end_j - (i + (m-n))|`, which
is just the distance from `end_j` to the right diagonal. Therefore,  `matrix(i, end_j) + |end_j - (i + (m-n))|` is a
_lower bound_ on the final number of edits required to transform one string into the other.  We require this lower bound
to be no greater than `max_edits`, that is, `matrix(i, end_j) + |end_j - (i + (m-n))| <= max_edits`. We can compute the LHS
value explicitly at each row, and if `end_j` is too large, we decrement it and recheck the inequality and continue
decrementing `end_j` until the inequality holds. As an exercise, verify to yourself that this inequality tells us we
should initialize `end_j` to `(max_edits + m-n) / 2`, the same formula from the previous section. Consequently, this
algorithm is at least as efficient as having a band of constant width `(max_edits + m-n)  + 1`.

By a symmetric argument, we require `matrix(i, end_j) + |(i + (m-n)) - start_j| <= max_edits`. (Try to write it out yourself as an exercise using the previous paragraph as a model.) After computing each row, the update of `start_j` and `end_j` looks something like this:

```c++
// See if we can make the band narrower based on the row just computed.
while( end_j > 0 && row[end_j] + std::abs(end_j - i - m-n) > max ){
    end_j--;
}
// Increment for next row
end_j = std::min(m, end_j + 1);

// The starting point is a little different. It is "sticky" unless we can prove it can shrink.
// Think of it as, both start and end do the most conservative thing.
while( start_j <= end_j && std::abs(i + m-n - start_j) + row[start_j] > max ) {
    start_j++;
}
```

If ever `start_j > end_j` (the case of an "empty band") before the algorithm ends, we know we will exceed `max_edits` and can stop computing.

This needs to be modified slightly for Damerau–Levenshtein edit distance, which only differs by allowing transpositions, because Damerau–Levenshtein needs to see out an additional column to detect transpositions. So just subtract 1 from the LHS in both `start_j` and `end_j` inequalities.

Very few implementations use this optimal band narrowing, but it is out there in a few places. As my advisor would say, it is well-known by those who know it.

### Example

Here is an example of what the computation looks like for the vanilla algorithm, the constant width banded algorithm, and the optimally banded algorithm. The `→` means insertion, `↓` deletion, `↘` substitution, and `⇘` means the characters match (no penalty). Of course, the real algorithm generally does not store the entire matrix, nor does it keep track of which edits lead to which values for each cell.

| Algorithm           | Number of Cells Computed |
|:--------------------|:-------------------------|
| Standard            | 224                      | 
| Constant Width Band | 133                      |
| Optimally Banded    | 57                       |

```aiignore
Standard Algorithm (max_cost = 5)
       A   a   p   t   o   s   y   a   x       g   r   y   p   u   s   
   0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11 →12 →13 →14 →15 →16
   ↓ ⇘                                                                 
A  1   0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11 →12 →13 →14 →15  
   ↓   ↓ ↘   ⇘                                           ⇘             
p  2   1   1   1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11 →12 →13 →14  
   ↓   ↓ ↘ ↓ ↘ ↓ ⇘                                                     
t  3   2   2   2   1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11 →12 →13  
   ↓   ↓ ↘ ↓ ↘ ↓   ↓ ↘   ↘   ⇘                       ⇘                 
y  4   3   3   3   2   2 → 3   3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11 →12  
   ↓   ↓ ↘ ↓ ↘ ↓   ↓ ↘ ↓ ⇘       ↘   ↘   ↘   ↘   ↘   ↘   ↘   ↘   ⇘     
s  5   4   4   4   3   3   2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11  11  
   ↓   ↓ ⇘   ↘ ↓   ↓ ↘ ↓   ↓ ↘   ⇘                                     
a  6   5   4 → 5   4   4   3   3   3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11  
   ↓   ↓   ↓ ↘     ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓ ⇘                                 
x  7   6   5   5   5   5   4   4   4   3 → 4 → 5 → 6 → 7 → 8 → 9 →10  
   ↓   ↓   ↓ ↘ ↓ ↘ ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓   ↓ ↘   ⇘                         
g  8   7   6   6   6   6   5   5   5   4   4   4 → 5 → 6 → 7 → 8 → 9  
   ↓   ↓   ↓ ↘ ↓ ↘ ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓ ⇘                     
r  9   8   7   7   7   7   6   6   6   5   5   5   4 → 5 → 6 → 7 → 8  
   ↓   ↓   ↓ ↘ ↓ ↘ ↓ ↘ ↓   ↓ ⇘   ↘ ↓   ↓ ↘ ↓ ↘ ↓   ↓ ⇘                 
y 10   9   8   8   8   8   7   6 → 7   6   6   6   5   4 → 5 → 6 → 7  
   ↓   ↓   ↓ ⇘   ↘ ↓ ↘ ↓   ↓   ↓ ↘     ↓ ↘ ↓ ↘ ↓   ↓   ↓ ⇘             
p 11  10   9   8 → 9   9   8   7   7   7   7   7   6   5   4 → 5 → 6  
   ↓   ↓   ↓   ↓ ↘   ↘ ↓   ↓   ↓ ↘ ↓ ↘ ↓ ↘ ↓ ↘ ↓   ↓   ↓   ↓ ↘   ↘     
i 12  11  10   9   9 →10   9   8   8   8   8   8   7   6   5   5 → 6  
   ↓   ↓   ↓   ↓ ↘ ↓ ↘     ↓   ↓ ↘ ↓ ↘ ↓ ↘ ↓ ↘ ↓   ↓   ↓   ↓ ⇘   ↘     
u 13  12  11  10  10  10  10   9   9   9   9   9   8   7   6   5 → 6  
   ↓   ↓   ↓   ↓ ↘ ↓ ↘ ↓ ⇘     ↓ ↘ ↓ ↘ ↓ ↘ ↓ ↘ ↓   ↓   ↓   ↓   ↓ ⇘     
s 14  13  12  11  11  11  10  10  10  10  10  10   9   8   7   6   5  

Constant Width Banded Algorithm (max_cost = 5)
       A   a   p   t   o   s   y   a   x       g   r   y   p   u   s   
   0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11 →12 →13 →14 →15 →16
   ↓ ⇘                                                                 
A  1   0 → 1 → 2 → 3 → 4 → 5   .   .   .   .   .   .   .   .   .   .  
   ↓   ↓ ↘   ⇘                                                         
p  2   1   1   1 → 2 → 3 → 4 → 5   .   .   .   .   .   .   .   .   .  
   ↓   ↓ ↘ ↓ ↘ ↓ ⇘                                                     
t  3   2   2   2   1 → 2 → 3 → 4 → 5   .   .   .   .   .   .   .   .  
   ↓   ↓ ↘ ↓ ↘ ↓   ↓ ↘   ↘   ⇘                                         
y  4   3   3   3   2   2 → 3   3 → 4 → 5   .   .   .   .   .   .   .  
   ↓   ↓ ↘ ↓ ↘ ↓   ↓ ↘ ↓ ⇘       ↘   ↘   ↘                             
s  5   4   4   4   3   3   2 → 3 → 4 → 5 → 6   .   .   .   .   .   .  
   ↓   ↓ ⇘   ↘ ↓   ↓ ↘ ↓   ↓ ↘   ⇘                                     
a  6   5   4 → 5   4   4   3   3   3 → 4 → 5 → 6   .   .   .   .   .  
   ↓       ↓ ↘     ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓ ⇘                                 
x  7   .   5   5   5   5   4   4   4   3 → 4 → 5 → 6   .   .   .   .  
   ↓         ↘ ↓ ↘ ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓   ↓ ↘   ⇘                         
g  8   .   .   6   6   6   5   5   5   4   4   4 → 5 → 6   .   .   .  
   ↓             ↘ ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓   ↓ ↘ ↓ ↘ ↓ ⇘                     
r  9   .   .   .   7   7   6   6   6   5   5   5   4 → 5 → 6   .   .  
   ↓                ↘  ↓   ↓ ⇘   ↘ ↓   ↓ ↘ ↓ ↘ ↓   ↓ ⇘                 
y 10   .   .   .   .   8   7   6 → 7   6   6   6   5   4 → 5 → 6   .  
   ↓                       ↓   ↓ ↘     ↓ ↘ ↓ ↘ ↓   ↓   ↓ ⇘             
p 11   .   .   .   .   .   8   7   7   7   7   7   6   5   4 → 5 → 6  
   ↓                           ↓ ↘ ↓ ↘ ↓ ↘ ↓ ↘ ↓   ↓   ↓   ↓ ↘   ↘     
i 12   .   .   .   .   .   .   8   8   8   8   8   7   6   5   5 → 6  
   ↓                               ↓     ↘ ↓ ↘ ↓   ↓   ↓   ↓ ⇘   ↘     
u 13   .   .   .   .   .   .   .   9   8 → 9   9   8   7   6   5 → 6  
   ↓                                   ↓   ↓   ↓   ↓   ↓   ↓   ↓ ⇘     
s 14   .   .   .   .   .   .   .   .   9 →10  10   9   8   7   6   5  

Optimally Banded Algorithm (max_cost = 5)
       A   a   p   t   o   s   y   a   x       g   r   y   p   u   s   
   0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 →10 →11 →12 →13 →14 →15 →16
   ↓ ⇘                                                                 
A  1   0 → 1 → 2 → 3   .   .   .   .   .   .   .   .   .   .   .   .  
   ↓   ↓ ↘   ⇘                                                         
p  2   1   1   1 → 2 → 3   .   .   .   .   .   .   .   .   .   .   .  
   ↓   ↓ ↘ ↓ ↘ ↓ ⇘                                                     
t  3   2   2   2   1 → 2 → 3   .   .   .   .   .   .   .   .   .   .  
   ↓     ↘ ↓ ↘ ↓   ↓ ↘   ↘   ⇘                                         
y  4   .   3   3   2   2 → 3   3   .   .   .   .   .   .   .   .   .  
   ↓               ↓ ↘ ↓ ⇘       ↘                                     
s  5   .   .   .   3   3   2 → 3 → 4   .   .   .   .   .   .   .   .  
   ↓                 ↘ ↓   ↓ ↘   ⇘                                     
a  6   .   .   .   .   4   3   3   3 → 4   .   .   .   .   .   .   .  
   ↓                       ↓ ↘ ↓ ↘ ↓ ⇘                                 
x  7   .   .   .   .   .   4   4   4   3 → 4   .   .   .   .   .   .  
   ↓                             ↘ ↓   ↓ ↘   ⇘                         
g  8   .   .   .   .   .   .   .   5   4   4   4   .   .   .   .   .  
   ↓                                   ↓ ↘ ↓ ↘ ↓ ⇘                     
r  9   .   .   .   .   .   .   .   .   5   5   5   4   .   .   .   .  
   ↓                                         ↘ ↓   ↓ ⇘                 
y 10   .   .   .   .   .   .   .   .   .   .   6   5   4   .   .   .  
   ↓                                               ↓   ↓ ⇘             
p 11   .   .   .   .   .   .   .   .   .   .   .   6   5   4   .   .  
   ↓                                                   ↓   ↓ ↘         
i 12   .   .   .   .   .   .   .   .   .   .   .   .   6   5   5   .  
   ↓                                                       ↓ ⇘         
u 13   .   .   .   .   .   .   .   .   .   .   .   .   .   6   5   .  
   ↓                                                           ↓ ⇘     
s 14   .   .   .   .   .   .   .   .   .   .   .   .   .   .   6   5
```

Notice that each banded algorithm computes the same numbers at the same positions as the previous algorithm when it computes the value for the cell at all. This is not necessarily true of all banded implementations, but it is true for variants that use the one row optimization with the optimization of omitting sentinel values, which I describe below.

## Micro Optimizations

The optimizations in this section will give you, say, 2% here, or 5% there. Or they might make it slower on your
machine. Different architectures behave differently. You need to benchmark and measure in order to know how it will
behave on your machine. You would only experiment with these optimizations if you are interested in squeezing out the
last drop of performance from your algorithm. On my machine, I was able to eek out another 10-15% by implementing all of these that apply to my use case and platform.

These tiny optimizations probably don't matter for most use cases. But if you have a performance critical application, these micro optimizations in aggregate might matter to you. In fact, some people adopt PostGreSQL specifically because of its fast fuzzy string matching and string searching. MySQL doesn't have a built-in edit distance function. That is why I wrote a MySQL UDF to implement one--and it's faster than the one in PostreSQL. If this feature influences adoption, well, maybe worrying about tiny increases in performance is worthwhile. 

### Don't initialize sentinels

Intuition dictates that you should initialize the cells immediately to the left or right of the band with `max_edits + 1`, like so:

```c++
if (start_j > 1) row[start_j-1] = max + 1;
if (end_j   < m) row[end_j+1]   = max + 1;
```

The intuition is that these cells will be _read_, the first one when computing `matrix(i, start_j)`, and the second one when computing `matrix(i+1, end_{j+1} - 1)`. (The notation is a little clumsy here. The quantity `end_{j+1}` is meant to be the last column computed on row `i+1`.) Since we know the real values of `matrix(i, start_j - 1)` and `matrix(i, end_j + 1)` are too large to contribute to the final edit distance, and since we don't actually want to _compute_ those values, we need (it seems) to put something there that the algorithm can safely read that we know will force the cell to not participate in the final answer.

However, if you are using the one row memory optimization described below, then the values remaining in these cells are:

- on the left hand side, the value of `matrix(i-1, start_{j - 1})` that was already proved to result in exceeding `max_edits` in order for us to increment `start_j` to its current position, or, in the case of `start_j==1`, the value `i`, which is the largest edit distance possible on row `i`; and
- on the right hand side, the value of `matrix(i-1, end_j)` that was already proved to result in exceeding `max_edits` in order for us to decrement `end_j` to its current position, or, in the case that `end_j` is as large as it's ever been, the value `i`, which is the largest edit distance possible on row `i`.

In all cases, neither of these values will interfere with your final computation, as they are already too large to make a difference. A similar argument works for the two row Damerau–Levenshtein algorithm.

I do not claim to be the first person to make this observation, because that is unlikely given how much study this algorithm has received, but I have never seen this optimization anywhere else, probably because the one row memory optimization is so rare.

### Lazy / Unconditional Initialization of First Column

The standard algorithm initializes the first column (index zero) with `i`, which is the number of deletions required to transform the second string into the empty string. You only need to do this, of course, if `start_j == 1`, as otherwise that value will never be accessed. On my machine, initializing this value conditionally or unconditionally makes no difference--it's a complete wash.

```c++
if (start_j == 1) // This line seems to make no difference.
        row[0] = i;
```

But this is exactly the kind of thing you should check yourself.

### Max and Min Functions

Some people like to define their own maximum and minimum functions like so:

```c++
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
```

On my machine `std::max` and `std::min` were measurably faster, though not by very much. Likewise, the STL provides multi-argument variants of these functions: `std::min({a, b, c})`. On some architectures this might provide a benefit.

### Make The First String The Longest / Shortest String

On your architecture you might see a benefit to forcing the longest string to be the source and the shortest string to be the target--or the other way around! I saw a very small but measurable difference. But if there is no difference, it might be helpful to do anyway just to make things easier to reason about. For example, if you always have `m >= n`, then `m-n` is non negative, which might save you one use of absolute value. This won't make a difference in the running time, but I found it to ease the cognitive burden a little bit while studying the problem.

### Use `end_j` Instead of `i + right_band_width`

Since we have derived the value of `end_j` by reasoning about a maximum distance from the diagonal, it is tempting to
keep track of the right maximum distance and the left maximum distance and then let `j` range from `i - left_band_width`
to `i + right_band_width`. In other words:

```c++
// The inner loop over columns
for(j = i - left_band_width; j <= i + right_band_width; j++){
  // compute `matrix(i,j)`
}
```

Instead of keeping track of relative distance from the diagonal, it's slightly more efficient to keep track of the
absolute position of the start and end. Apparently all of those additions and subtractions do add up enough to make a
measurable difference.

### Precomputing Unicode Character Lengths

If you have to support UTF-8 encoded Unicode characters, you need to account for the byte length of each character, which is not constant. A simple workaround is just to convert to a fixed length representation like UCS-2 (the fixed width subset of UTF-16) or UCS-4 (A.K.A. UTF-32). But if you are looking at optimizations as silly as those in this section, you don't want to do that, and it's not very hard to account for variable width characters anyway. 

The implementation in PostgresSQL, which is known for its efficiency, has a fast path for the case of no multibyte characters and, for the multibyte character case, builds a cache of character lengths before entering the main loop.


## Memory Optimizations

The matrix for two strings of lengths `m` and `n` requires `(m+1)(n+1)` bytes of memory. So strings of length 100
already need ~10KB of memory. This is certainly feasible with modern hardware, but the memory requirement grows quickly
with string length. We can do much better. In general, these memory optimizations don't affect speed, at least for
reasonably small strings, except for any benefit from the micro optimization of not initializing sentinel values (see above).

It is easy to see that the computation of the next row of the matrix depends only on data in the previous and current
rows. A common optimization, then, is to only keep two rows of the matrix in memory at any given time, one row to hold
the previous row, and another to hold the row currently being computed. This obviously requires `2(m+2)` bytes, where,
if we prefer, we can choose `m` to be the length of the shortest string--though which string length you use doesn't matter in practice.

Similarly, Damerau–Levenshtein variants might keep three rows.

### Single Row Levenshtein

Much less common, but just as easy to implement, is to only keep _one_ row in memory, and overwrite each cell of that row as the current row is being computed. Writing to the cell for `matrix(i, j)` overwrites the data in the cell for `matrix(i-1, j)`, which is needed for the computation of the very next cell at `matrix(i, j+1)`. But after that, the data is no longer needed. Therefore, we can store the value of `matrix(i-1, j)` in a temporary variable before overwriting it and use the temporary variable for the computation of the next cell. (The data in `matrix(i, j-1)` is not a problem, of course, because it was computed in the previous iteration and lives in the previous array position.) This obviously requires only `(m+1)` bytes of storage, half the memory required for the two row optimization. Whether this advantage is worth the minor added complexity is a matter of opinion, but it does enable the micro optimization of not initializing the sentinel values at either end of the band.

One can do even better in banded variants of the algorithm. One needs only a number of bytes equal to the band width. However, only in the most specialized of circumstances would any sane person implement this. I've never seen it done. I will leave the implementation details as an exercise to the reader.

### Two Row Damerau–Levenshtein

I do not claim to be the first person to invent this, because I think that is unlikely given the tremendous amount of attention this algorithm has received, but I have never seen anyone else implement this technique.

Following the pattern of the single row Levenshtein variant, we extend it to the two row Damerau–Levenshtein variant. We call our two rows `previous` and `current`, and we refer to the `(row, col) = (i, j)` entry in the (now unrealized) matrix of the vanilla algorithm as `matrix(i,j)` as before. Our argument that this algorithm does what we want it to will be an inductive argument. First the inductive step.

At the start of the computation of cell `matrix(i, j)` assume the following is true:
```
current[p]  = ╭  matrix(i, p)     for p < j
              ╰  matrix(i-1, p)   for p >= j
previous[p] = ╭  matrix(i-1, p)   for p < j - 2
              ╰  matrix(i-2, p)   for p >= j - 2
previous_cell          = matrix(i-1, j-1)
previous_previous_cell = matrix(i-1, j-2)
```

To compute `matrix(i, j)`, we need the following data:

```
matrix(i  , j-1) = current[j-1]
matrix(i-2, j-2) = previous[j-2]
matrix(i-1, j-1) = previous_cell
matrix(i-1, j  ) = current[j]
```

The RHS of these equations show that, given the initial facts, we indeed have these values available, so we are indeed able to compute the value in `matrix(i, j)`. To establish the induction step, we need to show that we can update `current`, `previous`, `previous_cell`, and the tediously named `previous_previous_cell` to maintain the invariant for the start of the next iteration.

For convenience, we introduce another temporary variable `current_cell`. We compute `current_cell = matrix(i, j)` and then perform the update for the next iteration (for `j+1`):

```
previous[j-2]          <-- previous_previous_cell := matrix(i-1, j-2),  only if j > 1
previous_previous_cell <-- previous_cell          := matrix(i-1, j-1)
previous_cell          <-- current[j]             := matrix(i-1, j)
current[j]             <-- current_cell           := matrix(i, j)
```

It is evident from this update procedure that we can indeed maintain the invariant. To establish the base case of the induction argument, you just need to show that there's a way to initialize everything, which is trivial and left as an exercise for the reader.

### One Row Damerau–Levenshtein

I published an early attempt at a _one_ row Damerau–Levenshtein variant several years ago in which I attempted to store
the information from the `previous` array in a single variable. If you squint, wave your hands a little bit, and don't
test the algorithm on a lot of cases with transpositions, the algorithm looks like it works. A quick look around the
internet shows that a few people picked up this algorithm, apparently trusting, perhaps because of my credentials, that
I had proven it correct, or at least rigorously tested it. *The algorithm is incorrect.* In retrospect, I wish I had labeled that algorithm as
experimental and still in need of peer review. In my defense, I didn't make any claims about that algorithm at all, its
fitness, or its peer review status--in fact I explicitly warned people not to trust code they find on the internet and
to do their own testing.

I am not aware of a way to compute Damerau–Levenshtein (Levenshtein with transpositions) in a single row.

I reiterate my warning to _not trust code you find on the internet_ without doing your own review and testing. The desire to communicate that message especially in the context of Levenshtein edit distance implementations is actually what motivated me to publish anything publicly in the first place. I have found a lot of widely used implementations that are susceptible to buffer overflow and DoS attacks. I shudder to think of how much infrastructure is running these implementations on public facing servers.

## More Esoteric Optimizations

### Exploiting the Data Dependencies

The data dependence goes from top left to bottom right. Thus, the cells on the lines `i = -j + b` can be computed in
parallel. (In fact, more is true: The cells of `matrix(i, *)` can be computed at the same time as the cells of  `matrix(i+1, *)` so long as  `matrix(i, j)` is computed before  `matrix(i+1, j)`.) This observation is the foundation for most GPU, SIMD, and multithreaded implementations. These variants
almost certainly perform worse than the variants discussed here for small strings (on the order of 40 characters or so)
because of overhead of shunting data around in memory. They are useful for very long strings. I haven't bothered
implementing any of these, because my use case is small strings only.

But that also means that I haven't _shown_ conclusively that those variants are not faster than the variants I have implemented, and our mantra is _benchmark and measure on your machine. YOUR MILEAGE MAY VARY._

### Cache Efficiency

Modern CPUs have several layers of data cache to improve the speed of memory access. If you can organize your problem in such a way as to have your memory access patterns take the best advantage of the processor's cache, you can improve performance. Zhao and Sahni[^1] have analyzed this algorithm with respect to a cache model and have verified experimentally that their variant of the algorithm that computes the matrix in strips the width of the cache line outperforms the standard algorithm, especially in the parallel case. Their test set consists of strings thousands of characters long. A cache line on present day processors is typically 64 bytes (128 bytes on my Apple Silicon Mac). If your strings are smaller than that, this strategy doesn't apply.

[^1]: Zhao and Sahni BMC Bioinformatics 2019, 20(Suppl 11):277 https://doi.org/10.1186/s12859-019-2819-0

### Alternative Algorithms Using Preprocessing

You can do better than these algorithms if you are able to preprocess your list of strings to be searched. Using a strategy that preprocesses the haystack is the
standard approach taken for spelling correction and word prediction algorithms. [Here is a very brief
overview](https://www.geeksforgeeks.org/data-structure-dictionary-spell-checker/) to get you started. The most common
strategy uses a data structure called a [trie](https://en.wikipedia.org/wiki/Trie) (frustratingly pronounced "tree", but
I refuse to do so). There are other strategies. But of course this article is about _this_ algorithm, not those other algorithms.

Be skeptical of claims about any algorithm that is several orders of magnitude better than any other algorithm. Maybe it is and maybe
it isn't for your use case. You will have to _benchmark and measure_ to be sure. The best strategy for your use case
will depend on your specific constrains on memory/disk space, speed requirements, available hardware, and so forth.

