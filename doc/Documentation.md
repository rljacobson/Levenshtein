This is just verbiage or information that should exist somewhere but doesn't yet.

# Gotchas

 - "Edit distance" for optimized algorithms are not symmetric. It is possible for `distance(string1, string2, max) != distance(string2, string1, max)` for some distance function `distance` and strings `string1` & `string2`.
   - For example, `DAMLEVLIM(string1, string2, max)` might "bail early" and return `max+1`, while `DAMLEVLIM(string1, string1, max)` (strings in the reverse order) might compute the actual distance on its last step and return the real Damerau–Levenshtein edit distance.
 - "Edit distance" for some optimized algorithms is dependent on the row order.
   - For example, `DAMLEVMIN(•, •, max)` keeps track of the smallest edit distance it has computed so far, call it `current_min`. If during the course of computation of a distance it can prove that the distance will be greater than `current_min`, `DAMLEVMIN` will return `max+1`. 


# Which algorithm to choose?

The (ordinary) *Levenshtein edit distance* is the minimum number of insertion, deletion and substitution operations needed to transform one string into the other. It is a measurement of similarity between two strings. *Damerau–Levenshtein edit distance* does the same but adds the transposition operation. On top of several optimizations to the standard Wagner–Fischer dynamic programming algorithm, the primary optimization strategy is to "bail early" when we can determine the edit distance will be greater than some maximum value.

If you want the true Damerau–Levenshtein edit distance, use `DAMLEV`, `DAMLEV2D`, or `DAMLEVP`.

| Function   | Description                                                                                                            | Symmetric | Precise |
|:-----------|:-----------------------------------------------------------------------------------------------------------------------|:---:|:--:|
| `DAMLEV2D` | "Standard" algorithm with no optimizations. Useful for benchmarking and testing against other algorithms for accuracy. | Yes | Yes |
| `DAMLEV`   | Algorithm with several optimizations applied.                                                                          | Yes | Yes |
| `DAMLEVP`  | Algorithm with several optimizations applied. Returns edit distance "normalized" against string length.                | Yes | Yes |

The remaining algorithms stop computing when the count of edits exceeds some predetermined maximum distance. This can save a significant amount of computation time, as the number of steps these algorithms require is less than or equal to the maximum distance. If you are only interested in, say, which strings are within five edits, use these. 

| Function    | Description                                                                                                                                                        | Symmetric | Precise                                            |
|:------------|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------|:---:|:---------------------------------------------------|
| `DAMLEVLIM` | Only computes distances no more than the user supplied max.                                                                                                        | No | Only for distances less than given max             |
| `DAMLEVMIN` | The max distance is set to `min(max distance, current distance)`, so that the max distance deceases monotonically. Use to find smallest distance / closest string. | No | Only for smallest distance, if less than given max |

The max distance in the `DAMLEVMIN` function changes dynamically, decreasing to the smallest edit distance computed so far if it is smaller than max distance. This is useful in the common case that you are interested only in strings with the smallest edit distance to a particular (constant) string. (The function itself doesn't require one of the strings to be constant, so other use cases are possible.)

