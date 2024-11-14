#pragma once

#include "../src/common.h"
#include <mysql.h>

// #define UDF_SIGNATURES(algorithm) int MACRO_CONCAT(algorithm, _init)(UDF_INIT *initid, UDF_ARGS *args, char *message); \
//     long long algorithm(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error); \
//     void MACRO_CONCAT(algorithm, _deinit)(UDF_INIT *initid);

// Levenshtein
UDF_SIGNATURES(lev)
UDF_SIGNATURES(levlim)
UDF_SIGNATURES(levlimopt)
UDF_SIGNATURES(levmin)

// Damerau–Levenshtein
UDF_SIGNATURES(damlev2D)
UDF_SIGNATURES(damlev)
UDF_SIGNATURES(damlevlim)
UDF_SIGNATURES(damlevmin)

// The next two are special, as they return a `double` instead of a `long long`.
UDF_SIGNATURES_TYPE(damlevp, double)
UDF_SIGNATURES_TYPE(damlevminp, double)

// int damlevminp_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
// double damlevminp(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
// void damlevminp_deinit(UDF_INIT *initid);

// int damlevp_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
// double damlevp(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
// void damlevp_deinit(UDF_INIT *initid);

// Benchmarking only
UDF_SIGNATURES(noop)

