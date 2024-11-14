/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    31 July 2019 - Robert Jacobson

    This is a no-op UDF function that just returns as soon as it's called.
    It's purpose is to provide a lower bound for benchmarking purposes.

    <hr>

    Copyright (C) 2019 Robert Jacobson. Released under the MIT license.

    Based on "Iosifovich", Copyright (C) 2019 Frederik Hertzum, which is
    licensed under the MIT license: https://bitbucket.org/clearer/iosifovich.

    The MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include <iostream>

#include "common.h"

// Limits
#ifndef NOOP_BUFFER_SIZE
// 640k should be good enough for anybody.
#define NOOP_BUFFER_SIZE 512ull
#endif
constexpr long long NOOP_MAX_EDIT_DIST = std::max(0ull,
                                                       std::min(16384ull, NOOP_BUFFER_SIZE));

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
    NOOP_ARG_NUM_ERROR[] = "Wrong number of arguments. NOOP() requires three arguments:\n"
                                "\t1. A string\n"
                                "\t2. A string\n"
                                "\t3. A maximum distance (0 <= int < ${NOOP_MAX_EDIT_DIST}).";
constexpr const auto NOOP_ARG_NUM_ERROR_LEN = std::size(NOOP_ARG_NUM_ERROR) + 1;
constexpr const char NOOP_MEM_ERROR[] = "Failed to allocate memory for NOOP"
                                             " function.";
constexpr const auto NOOP_MEM_ERROR_LEN = std::size(NOOP_MEM_ERROR) + 1;
constexpr const char
    NOOP_ARG_TYPE_ERROR[] = "Arguments have wrong type. NOOP() requires three arguments:\n"
                                 "\t1. A string\n"
                                 "\t2. A string\n"
                                 "\t3. A maximum distance (0 <= int < ${NOOP_MAX_EDIT_DIST}).";
constexpr const auto NOOP_ARG_TYPE_ERROR_LEN = std::size(NOOP_ARG_TYPE_ERROR) + 1;


UDF_SIGNATURES(noop)


struct DamLevConstMinData {
    int max;
    int *buffer; // Takes ownership of this buffer

    DamLevConstMinData(int max, int *buffer): max(max), buffer(buffer){}

    ~DamLevConstMinData(){ delete this->buffer; }
};

[[maybe_unused]]
int noop_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {

    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, NOOP_ARG_NUM_ERROR, NOOP_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT ||
        args->arg_type[2] != INT_RESULT) {
        strncpy(message, NOOP_ARG_TYPE_ERROR, NOOP_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Initialize persistent data
    int* buffer = new (std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    DamLevConstMinData *data = new (std::nothrow) DamLevConstMinData(DAMLEV_MAX_EDIT_DIST, buffer);
    // If memory allocation failed
    if (!buffer || !data) {
        strncpy(message, NOOP_MEM_ERROR, NOOP_MEM_ERROR_LEN);
        return 1;
    }
    initid->ptr = reinterpret_cast<char*>(data);


    // There are two error states possible within the function itself:
    //    1. Negative max distance provided
    //    2. Buffer size required is greater than available buffer allocated.
    // The policy for how to handle these cases is selectable in the CMakeLists.txt file.
#if defined(RETURN_NULL_ON_BAD_MAX) || defined(RETURN_NULL_ON_BUFFER_EXCEEDED)
    initid->maybe_null = 1;
#else
    initid->maybe_null = 0;
#endif

    return 0;
}

[[maybe_unused]]
void noop_deinit(UDF_INIT *initid) {
    // As `DamLevMinPersistant` owns its buffer, `~DamLevMinPersistant` handles buffer deallocation.
    delete reinterpret_cast<DamLevConstMinData*>(initid->ptr);
}

[[maybe_unused]]
long long noop(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, [[maybe_unused]] char *error) {

#ifdef PRINT_DEBUG
    std::cout << "noop" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[8];
#endif

    // Retrieve the persistent data.
    DamLevConstMinData *data = reinterpret_cast<DamLevConstMinData *>(initid->ptr);
    long long max = std::min(
            *(reinterpret_cast<long long *>(args->args[2])),
            static_cast<long long>(data->max)
    );
    int *buffer = data->buffer;

    // Validate max distance and update.
    // This code is common to algorithms with limits.
#include "validate_max.h"

    // The pre-algorithm code is the same for all algorithm variants. It handles
    //     - basic setup & initialization
    //     - trimming of common prefix/suffix
#include "prealgorithm.h"

#ifdef CAPTURE_METRICS
    metrics.total_time += call_timer.elapsed();
#endif

    return 0;
}
