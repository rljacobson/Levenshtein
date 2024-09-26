#pragma once

#define NOMINMAX // msvc++ incompatibility

#include <algorithm>
#include <vector>
#include <string>
#include <string_view>
#include <cstring>
#include <iterator>
#include <numeric>

#include "mysql.h"


// Limits
#ifndef DAMLEV_BUFFER_SIZE
// 640k should be good enough for anybody.
#define DAMLEV_BUFFER_SIZE 512ull
#endif
constexpr long long DAMLEV_MAX_EDIT_DIST = std::max(0ull, std::min(16384ull, DAMLEV_BUFFER_SIZE));


#ifdef PRINT_DEBUG
#include <iostream>
#endif

#ifdef _WIN64
// msvc++ incompatibility
#define UNUSED
#else
// Silences warning in gcc/clang.
#define UNUSED __attribute__((unused))
#endif
