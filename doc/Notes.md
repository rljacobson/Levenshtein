# Table of Algorithms

| Name       | Description                               | Use case                        | delete? |
|:-----------|:------------------------------------------|:--------------------------------|:-------:|
| damlev     | 2 row                                     | Accurate distances in all cases |   no    |
| damlev2D   | 2D, no trimming, no max                   | Benchmark and unit tests only   |   no    |
| damlevlim  | 2 row, banded, max early exit             | Compute distances less than k   |   no    |
| damlevmin  | 2 row, banded, max early exit, tracks min | Find closest to const string    |   no    |
| damlevp    | 2 row, banded, max early exit             | Similarity Score at least p     |   no    |
| damlevpmin | replace with variant of damlevmin         | Find closest similarity         | replace |
| levlim     | 1 row, banded, max early exit             | Compute Edit Distance           |   no    |
| levlimopt  | 1 row, aggressive banding                 | Benchmarks only                 |   no    |
| noop       | does nothing                              | Benchmarks only                 |   no    |

# To Do

 - [ ] Change "policy" macros/variables to use string values so they are guaranteed to be mutually exclusive.
 - [ ] Use the CACHE STRING option for user configurable values in CMake.
 - [ ] Benchmark: make max edit distance configurable instead of hard coded.
 - [ ] One-off: make two strings configurable instead of hard coded.
 - [ ] Need to look at CMake files and decide how to clean them up a bit for linux and windows builds.
 - [ ] Test our include/mysql-8-0-39, put all mysql source code version in the include, this removes the need to use host system for build, allows for cross compiler, would also allow other verisons to be built. EDIT: This is tricky. We'll always be chasing new versions, different platforms. Using the system header is probably the right thing to do.
 - [ ] Policy for buffer exceeded is not implemented. We just return 0. However, policy is selectable in CMakeLists.txt.
 - [ ] In description of optimizations, discuss the condition `start_j = std::max(1, i - (effective_max - minimum_within_row))` and `start_j = std::max(1, i - effective_max)`.

# Changes Made

- [MySQL no longer uses](https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-1.html#mysqld-8-0-1-compiling)
  `my_bool`. Use `int` instead for all `*_init` functions.
- We do all our arithmetic in `int`, but the functions return `long long`, because that is the C++ datatype corresponding to MySQL's `INTEGER`. This isn't a problem.
- clang-tidy "unused" warnings are silences with `[[maybe_unused]]`, which is part of the standard as of C++17. We no longer use `__attribute__((unused))`.
- Factor out common "pre-algorithm" code that is the same across algorithms.


# Questions

 * Why initialize `DAMLEV_MAX_EDIT_DISTANCE` to the buffer size? Maybe use inverse function of that used in buffer size check, or possibly something else.
 * How does `benchmark` differ from `unittests`? ANSWER: Benchmark more closely simulates our use case for correcting scientific names. Also, `unittests` does setup and teardown for every call.


# Some results


## Narrowing band in banded variant

The standard banded algorithm only computes cells within `max+1` of the diagonal. More aggressive narrowing of the band is possible. For example, the condition `start_j = std::max(1, i - (effective_max - minimum_within_row))` is superior to `start_j = std::max(1, i - effective_max)` in terms of computing fewer cells and exiting early more often. However, this condition apparently increases branch misprediction and consequently performs worse that the standard band. 

Another band narrowing strategy is the following:

```c++
if(j > i && current_cell > effective_max && buffer[j] > effective_max) {
    // Don't bother computing the remainder of the band.
    buffer[j] = current_cell;
    if(j<=m) buffer[j+1] = max + 1; // Set sentinel.
    end_j = j;
    break;
}
```
