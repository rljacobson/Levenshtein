# To Do

 - [MySQL no longer uses](https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-1.html#mysqld-8-0-1-compiling) 
`my_bool`. Use `int` instead for all `*_init` functions.
 - Factor out common "pre-algorithm" code that is the same across algorithms.
 - Bring up bit-rotted algorithms
   - damlev
   - damvlevmin
   - damlevlim
   - damlevlimmin ?
   - damlevp
 - Reintroduce optimizations one-by-one
   - n/2 ("sliding window") limit on column ranges.
   - Single row buffer
 - Fix issue in LEV_SETUP where you need to change the number of arguments depending on the algorithm.
 - We do all our arithmetic in `int`, but the functions return `long long`, because that is the C++ datatype corresponding to MySQL's `INTEGER`.
 - clang-tidy "unused" warnings are silences with `[[maybe_unused]]`, which is part of the standard as of C++17. We no longer use `__attribute__((unused))`.
 - Change "policy" macros/variables to use string values so they are guaranteed to be mutually exclusive.
 - Use the CACHE STRING option for user configurable values in CMake.
 - Need to look at CMake files and decide how to clean them up a bit for linux and windows builds.  Also NOOP should likely only be in the tests, not the build verison.
 - Test our include/mysql-8-0-39, put all mysql source code verison in the include, this removes the need to use host system for build, allows for cross complier, would alos allow other verisons to be built. 

# Changes Made

- We do all our arithmetic in `int`, but the functions return `long long`, because that is the C++ datatype corresponding to MySQL's `INTEGER`.
- clang-tidy "unused" warnings are silences with `[[maybe_unused]]`, which is part of the standard as of C++17. We no longer use `__attribute__((unused))`.
- [MySQL no longer uses](https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-1.html#mysqld-8-0-1-compiling)
  `my_bool`. Use `int` instead for all `*_init` functions.
- Factor out common "pre-algorithm" code that is the same across algorithms.


# Questions

 * Why initialize `DAMLEV_MAX_EDIT_DISTANCE` to the buffer size?
 * How does `benchmark` differ from `unittests`? Answer: Benchmark more closely simulates our use case for correcting scientific names.



### Early Exits and Transpositions: Why Results May Differ

The `DAMLEVCONST` and other functions in this library are optimized for speed and efficiency by utilizing an early exit mechanism. This mechanism allows the function to terminate early when it becomes clear that the minimum edit distance has already exceeded a user-defined maximum (`PosInt`). In practical use cases, this is often desired since it can save computational resources when an exact edit distance is not required if it's already "too far off." However, there are a few subtleties to keep in mind due to how these early exits interact with string traversal and transpositions.

### Why Different Results for the Same Inputs?

The Damerau-Levenshtein distance algorithm processes strings row-by-row in a matrix. When the early exit condition (`column_min > effective_max`) is met, the function terminates immediately, avoiding further computation. This is intentional and often advantageous, especially in database queries where performance is crucial.

However, the traversal of the matrix can vary depending on the order of the input strings. This means that switching the `subject` and `query` parameters can lead to different termination points when the early exit is triggered. Consequently, you might observe different distances for `DAMLEVCONST(String1, String2, PosInt)` compared to `DAMLEVCONST(String2, String1, PosInt)`.

Importantly, neither result is wrong. The purpose of the early exit is to inform you that the edit distance has surpassed the threshold (`PosInt`), rather than to compute the precise distance in cases where it doesn’t matter. The algorithm correctly signals that the strings are more dissimilar than the maximum allowed difference, and the exact value becomes less relevant.

### How This Is Not an Issue

This behavior is not an error or a flaw in the `DAMLEVCONST` function. The early exit mechanism is a deliberate optimization designed to enhance performance in real-world applications, where knowing that a comparison exceeds a threshold is often sufficient. Here’s why this approach is beneficial and not a problem:

1. **Performance:** The early exit drastically improves efficiency, especially when processing large volumes of data, such as in database searches. It allows the function to stop once it’s clear that the strings differ too much, saving computational time and resources.

2. **Indication of Exceeding the Threshold:** The primary intent behind specifying a maximum edit distance (`PosInt`) is to find matches within a certain similarity. When the function exits early, it is correctly indicating that the distance between the two strings exceeds this limit. The exact distance isn't as important once it's known that the strings are beyond the allowed threshold.

3. **Consistent with Real-World Use:** In many use cases, especially in databases and search functionalities, you often only need to know if a string's edit distance is below a certain threshold. The early exit provides this information efficiently, which aligns with practical usage scenarios.

4. **Transpositions and Processing Order:** The Damerau-Levenshtein algorithm accounts for transpositions (swapping adjacent characters), but the order in which it processes the strings can affect how soon the early exit condition is met. Since transpositions are just one of the many operations considered (along with insertions, deletions, and substitutions), slight differences in processing order can influence the early termination. This does not imply a fault in the algorithm but rather reflects the nature of how it optimizes for speed.

### Summary

The `DAMLEVCONST` functions are designed to exit early when the edit distance surpasses the specified maximum. This can lead to slight differences in the reported distance when the order of the strings is swapped, especially in cases involving transpositions. However, this behavior is not an issue. The early exit correctly serves its purpose: to inform you when the edit distance is beyond the specified limit, which is often all you need to know. If an exact distance is necessary in every case, the early exit mechanism can be disabled, but doing so will reduce the algorithm’s efficiency.

By prioritizing speed and resource savings, `DAMLEVCONST` and related functions provide an effective tool for similarity searches and comparisons, especially when used with a maximum distance limit. This is by design and reflects a thoughtful trade-off between accuracy and performance in contexts where exact distances are not always critical.

### Example Matrices

#### Early Exit Example

When comparing `"erecta"` and `"rceaet"`:

|   | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|---|
| 0 | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
| r | 1 | 1 | 1 | 2 | 3 | 4 | 5 |
| c | 2 | 2 | 2 | 2 | 2 | 3 | 4 |
| e | 3 | 2 | 3 | 2 | 2 | 3 | 4 |
| a | 4 | 3 | 3 | 3 | 3 | 3 | 3 |
| e | 5 | 4 | 4 | 3 | 4 | 4 | 4 |
| t | 6 | 5 | 5 | 4 | 4 | 4 | 5 |

**Early exit:** The algorithm exits as the minimum distance in the last row (`4`) is the lowest distance (column_min) in the row and exceeds the `effective_max` (`3`).
`(column_min > effective_max) === True`

**Result:** `calculateDamLevDistance(erecta, rceaet) = 4`.

#### Full Computation Example

When comparing `"rceaet"` and `"erecta"`:

|   | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|---|
| 0 | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
| e | 1 | 1 | 2 | 2 | 3 | 4 | 5 |
| r | 2 | 1 | 2 | 3 | 3 | 4 | 5 |
| e | 3 | 2 | 2 | 2 | 3 | 3 | 4 |
| c | 4 | 3 | 2 | 2 | 3 | 4 | 4 |
| t | 5 | 4 | 3 | 3 | 3 | 4 | 4 |
| a | 6 | 5 | 4 | 4 | 3 | 4 | 5 |

**Result:** The entire matrix is computed, and the final edit distance is `5`.

### Conclusion

Both matrices indicate that the edit distance exceeds the specified maximum (`3`). The early exit mechanism ensures efficient processing by halting further computation once the distance is known to be beyond the threshold. The slight differences in results are a reflection of the algorithm's optimization, not a flaw.
