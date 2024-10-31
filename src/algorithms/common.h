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


// Limits. These are set elsewhere, but we include this here for any use case that doesn't care about these details and
// just wants to use the code.
#ifndef DAMLEV_BUFFER_SIZE
// 640k should be good enough for anybody.
#define DAMLEV_BUFFER_SIZE 4096ull
#endif
constexpr long long DAMLEV_MAX_EDIT_DIST = std::max(0ull, std::min(16384ull, DAMLEV_BUFFER_SIZE));


#ifdef PRINT_DEBUG
#include <iostream>
#endif


// Define set_error as an inline function
inline void set_error(char *error, const char *message) {
    strncpy(error, message, MYSQL_ERRMSG_SIZE);
}

