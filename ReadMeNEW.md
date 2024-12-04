# Blazingly Fast Damerau–Levenshtein Edit Distance UDF for MySQL

An extremely fast and efficient implementation of Levenshtein edit distance and Damerau–Levenshtein edit distance for
MySQL / MariaDB. This UDF is implemented in C++.

The _edit distance_ between two strings is defined as the minimum number of edits required to transform one string into
the other. The difference between _Levenshstein_ edit distance and _Damerau–Levenshtein_ edit distance is in what counts
as an "edit": Damerau–Levenshtein includes the transposition of two characters.

|                                      | Levenshstein | Damerau–Levenshtein |
|:-------------------------------------|:------------:|:-------------------:|
| insert a character                   |      X       |          X          |
| delete a character                   |      X       |          X          |
| substitute one character for another |      X       |          X          |
| swap two adjacent characters         |    &nbsp;    |          X          |

# Catalog of Functions

This UDF provides several different functions for different use cases.

| Function                                       | Description                                                  |
| :--------------------------------------------- | :----------------------------------------------------------- |
| `edit_dist(string1, string2)`                  | Computes the edit distance between two strings.<br> (Levenshtein edit distance, no transpositions) |
| `edit_dist_t(string1, string2)`                | Computes the edit distance between two strings, allowing transpositions.<br/> (Damerau-Levenshtein edit distance) |
| `bounded_edit_dist(string1, string2, bound)`   | Computes the edit distance between two strings if the distance is at most `bound`; otherwise returns `bound + 1`.<br/> (Levenshtein edit distance, no transpositions) |
| `bounded_edit_dist_t(string1, string2, bound)` | Computes the edit distance between two strings if the distance is at most `bound`; otherwise returns `bound + 1`.<br/> (Damerau-Levenshtein edit distance) |
| `min_edit_dist()`                              | Remembers the smallest edit distance seen so far during the query and uses it as an upper bound during the computation. |
| `similarity_t(string1, string2)`               | Computes a _normalized_ Damerau-Levenshtein percent **_similarity_** between two strings. |

- The suffix `_t` stands for *transpositions* and indicates the function counts swapping two adjacent characters as an edit (Damerau-Levenshtein edit distance).
- The prefix `bounded_` allows the algorithm to stop computing if it can prove the bound will be exceeded. This provides a significant performance improvement over the unbounded version, especially if you can give it a very small `bound`.
- The `min_`  functions remember the smallest edit distance seen so far in the search and use it as the upper bound as in the `bounded_` functions. Use this for searching for the closest match to a single particular string, *as it will give you orders of magnitude better performance*.<br><br>Because of how this algorithm works, the "distance" computed is only guaranteed to be accurate if it is the *smallest* distance computed during the query. 

- Similarity is a number from 0.0 to 1.0 interpreted as a percent similarity. Its advantage is that it is independent of string length. Similarity is computed by *normalizing* the edit distance by dividing it by the length of the longest string and subtracting that number from 100%: $100\% - \frac{\text{edit distance}}{\text{max}(\;\text{length}(\text{string1}),\; \text{length}(\text{string2})\;)}$
- There is no plain `similarity`, only `similarity_t`. If you want a similarity without transpositions, you can either compute it yourself using `edit_dist` and the formula for similarity, or you can request we add it.

## Limitations

* This implementation assumes characters are represented as 8 bit `char`'s on your platform. If you are using UTF-8 codepoints above 255 (i.e. outside of UCS-2), this function will not
  compute the correct edit distance between your strings.
* This function is case sensitive. If you need case insensitivity, you need to either compose this
  function with `LOWER`/`TOLOWER`, or adapt the code.
* By default, `BUFFER_SIZE` has a default maximum of 4096 bytes. You can configure this maximum by changing
  `BUFFER_SIZE` in `CMakeLists.txt`. See the Configuration section below for more details.

Any one of these limitations would be a good for a contributor to solve. Make a pull
request!


## 

# Invitation For Peer Review

I have tested this library extensively, but a single person is susceptible to making the same mistake over and over again. There is no substitute for quality peer review. I invite other people to test and critique this library and report back to me. 
