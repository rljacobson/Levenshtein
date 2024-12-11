#pragma once

#include "../src/common.h"
#include <mysql.h>

// #define UDF_SIGNATURES(algorithm) int MACRO_CONCAT(algorithm, _init)(UDF_INIT *initid, UDF_ARGS *args, char *message); \
//     long long algorithm(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error); \
//     void MACRO_CONCAT(algorithm, _deinit)(UDF_INIT *initid);

// Levenshtein
UDF_SIGNATURES(edit_dist)
UDF_SIGNATURES(bounded_edit_dist)
UDF_SIGNATURES(min_edit_dist)

// Damerau–Levenshtein
UDF_SIGNATURES(edit_dist_t_2d)
UDF_SIGNATURES(edit_dist_t)
UDF_SIGNATURES(bounded_edit_dist_t)
UDF_SIGNATURES(min_edit_dist_t)

// The next two are special, as they return a `double` instead of a `long long`.
UDF_SIGNATURES_TYPE(similarity_t, double)
UDF_SIGNATURES_TYPE(min_similarity_t, double)

// int min_similarity_t_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
// double min_similarity_t(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
// void min_similarity_t_deinit(UDF_INIT *initid);

// int similarity_t_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
// double similarity_t(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
// void similarity_t_deinit(UDF_INIT *initid);

// Benchmarking only
UDF_SIGNATURES(noop)

