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

# Questions

 * Why initialize `DAMLEV_MAX_EDIT_DISTANCE` to the buffer size?
 * How does `benchmark` differ from `unittests`? Answer: Benchmark more closely simulates our use case for correcting scientific names.
