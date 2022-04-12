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
