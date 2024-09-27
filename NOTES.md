# To Do

 - [MySQL no longer uses](https://dev.mysql.com/doc/relnotes/mysql/8.0/en/news-8-0-1.html#mysqld-8-0-1-compiling) 
`my_bool`. Use `int` instead for all `*_init` functions.
 - Solve the memory checksum issue. Appears to be a use after free. Bisect to determine when issue was introduced. 
Possibly dependent on random seed, as it is nondeterministic.
 - Reintroduce optimizations one-by-one


# Questions

 * How does `benchmark` differ from `unittests`?
 * What is the difference between `WORDCOUNT` and `LOOP` in `unittests.cpp`?
