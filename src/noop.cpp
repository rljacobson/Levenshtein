/*
Copyright (C) 2024 Robert Jacobson
Distributed under the MIT License. See License.txt for details.

This is a no-op UDF function that just returns as soon as it's called.
It's purpose is to provide a lower bound for benchmarking purposes.

*/

#include <iostream>

#include "common.h"

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
    NOOP_ARG_NUM_ERROR[] = "Wrong number of arguments. noop() requires three arguments:\n"
                                "\t1. A string\n"
                                "\t2. A string\n"
                                "\t3. A maximum distance (0 <= int < ${NOOP_MAX_EDIT_DIST}).";
constexpr const auto NOOP_ARG_NUM_ERROR_LEN = std::size(NOOP_ARG_NUM_ERROR) + 1;
constexpr const char NOOP_MEM_ERROR[] = "Failed to allocate memory for noop"
                                             " function.";
constexpr const auto NOOP_MEM_ERROR_LEN = std::size(NOOP_MEM_ERROR) + 1;
constexpr const char
    NOOP_ARG_TYPE_ERROR[] = "Arguments have wrong type. noop() requires three arguments:\n"
                                 "\t1. A string\n"
                                 "\t2. A string\n"
                                 "\t3. A maximum distance (0 <= int < ${NOOP_MAX_EDIT_DIST}).";
constexpr const auto NOOP_ARG_TYPE_ERROR_LEN = std::size(NOOP_ARG_TYPE_ERROR) + 1;


UDF_SIGNATURES(noop)


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
    // If memory allocation failed
    if (!buffer) {
        strncpy(message, NOOP_MEM_ERROR, NOOP_MEM_ERROR_LEN);
        return 1;
    }
    initid->ptr = reinterpret_cast<char*>(buffer);


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
    delete reinterpret_cast<int*>(initid->ptr);
}

[[maybe_unused]]
long long noop([[maybe_unused]] UDF_INIT *initid, [[maybe_unused]] UDF_ARGS *args, [[maybe_unused]] char *is_null, [[maybe_unused]] char *error) {

#ifdef PRINT_DEBUG
    std::cout << "noop" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[10];
#endif

#ifdef EXCLUDE_PREPROCESSING_FROM_NOOP
    #ifdef CAPTURE_METRICS
    metrics.call_count++;
    Timer call_timer;
    call_timer.start();
#endif
#ifdef CAPTURE_METRICS
    metrics.total_time += call_timer.elapsed();
#endif

    return 0;
#else
    // Retrieve the persistent data.
    int *buffer = reinterpret_cast<int *>(initid->ptr);
    long long max = *reinterpret_cast<long long *>(args->args[2]);
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
#endif
}
