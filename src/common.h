/*
Copyright (C) 2019 Robert Jacobson
Distributed under the MIT License. See LICENSE.txt for details.

Common definitions needed for every algorithm:
  - includes
  - macros
  - sanity limits on BUFFER_SIZE, DAMLEV_MAX_EDIT_DIST
  - `set_error(..)` function
  - UDF_SIGNATURES macro

*/

#pragma once

#define NOMINMAX // msvc++ incompatibility

#include <algorithm>
#include <vector>
#include <string>
#include <string_view>
#include <cstring>
#include <iterator>
#include <numeric>
#include <limits>
#include <climits>
#include <mysql.h>

#ifdef CAPTURE_METRICS
#include "metrics.hpp"
#include "benchtime.hpp"
#endif

/// Concatenate preprocessor tokens A and B without expanding macro definitions
/// (however, if invoked from a macro, macro arguments are expanded).
#ifndef MACRO_CONCAT_NO_EXPAND
#define MACRO_CONCAT_NO_EXPAND(A, B) A ## B
#endif

/// Concatenate preprocessor tokens A and B after macro-expanding them.
#ifndef MACRO_CONCAT
#define MACRO_CONCAT(A, B) MACRO_CONCAT_NO_EXPAND(A, B)
#endif

#define STRINGIFY(x) #x
#define AS_STRING(x) STRINGIFY(x)

/// Limits. These are set elsewhere, but we include this here for any use case that doesn't care about these details and
/// just wants to use the code.
#ifndef DAMLEV_BUFFER_SIZE
// 640k should be good enough for anybody.
#define DAMLEV_BUFFER_SIZE 4096ull
#endif
constexpr long long DAMLEV_MAX_EDIT_DIST = std::max(0ull, std::min(16384ull, DAMLEV_BUFFER_SIZE));

#ifdef PRINT_DEBUG
#include <iostream>
#endif

// Define set_error as an inline function
inline
void set_error(char *error, const char *message) {
    strncpy(error, message, MYSQL_ERRMSG_SIZE);
}

#define UDF_SIGNATURES_TYPE(algorithm, type) \
    extern "C" { \
        [[maybe_unused]] int MACRO_CONCAT(algorithm, _init)(UDF_INIT *initid, UDF_ARGS *args, char *message); \
        [[maybe_unused]] type algorithm(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);        \
        [[maybe_unused]] void MACRO_CONCAT(algorithm, _deinit)(UDF_INIT *initid);                             \
    }

#define UDF_SIGNATURES(algorithm) UDF_SIGNATURES_TYPE(algorithm, long long)
