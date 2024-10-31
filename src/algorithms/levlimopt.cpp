/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    17 January 2019

    This implementation is better than most others out there. It is extremely
    fast and efficient.
            __—R.__

    <hr>
    `LEVLIMOPT()` computes the Damarau Levenshtein edit distance between two strings when the
    edit distance is less than a given number.

    Syntax:

        LEVLIMOPT(String1, String2, PosInt);

    `String1`:  A string constant or column.
    `String2`:  A string constant or column to be compared to `String1`.
    `PosInt`:   A positive integer. If the distance between `String1` and
                `String2` is greater than `PosInt`, `LEVLIMOPT()` will stop its
                computation at `PosInt` and return `PosInt`. Make `PosInt` as
                small as you can to improve speed and efficiency. For example,
                if you put `WHERE LEVLIMOPT(...) < k` in a `WHERE`-clause, make
                `PosInt` be `k`.

    Returns: Either an integer equal to the edit distance between `String1` and `String2` or `k`,
    whichever is smaller.

    Example Usage:

        SELECT Name, LEVLIMOPT(Name, "Vladimir Iosifovich Levenshtein", 6) AS
            EditDist FROM CUSTOMERS WHERE  LEVLIMOPT(Name, "Vladimir Iosifovich Levenshtein", 6) <= 6;

    The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
    where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".

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
#include "common.h"
#include <iostream>

void printMatrix(const int* dp, int n, int m, const std::string_view& S1, const std::string_view& S2);

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        LEVLIMOPT_ARG_NUM_ERROR[] = "Wrong number of arguments. LEVLIMOPT() requires three arguments:\n"
                                    "\t1. A string\n"
                                    "\t2. A string\n"
                                    "\t3. A maximum distance (0 <= int < ${LEVLIMOPT_MAX_EDIT_DIST}).";
constexpr const auto LEVLIMOPT_ARG_NUM_ERROR_LEN = std::size(LEVLIMOPT_ARG_NUM_ERROR) + 1;
constexpr const char LEVLIMOPT_MEM_ERROR[] = "Failed to allocate memory for LEVLIMOPT"
                                             " function.";
constexpr const auto LEVLIMOPT_MEM_ERROR_LEN = std::size(LEVLIMOPT_MEM_ERROR) + 1;
constexpr const char
        LEVLIMOPT_ARG_TYPE_ERROR[] = "Arguments have wrong type. LEVLIMOPT() requires three arguments:\n"
                                     "\t1. A string\n"
                                     "\t2. A string\n"
                                     "\t3. A maximum distance (0 <= int < ${LEVLIMOPT_MAX_EDIT_DIST}).";
constexpr const auto LEVLIMOPT_ARG_TYPE_ERROR_LEN = std::size(LEVLIMOPT_ARG_TYPE_ERROR) + 1;

// Use a "C" calling convention.
extern "C" {
    [[maybe_unused]] int levlimopt_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    [[maybe_unused]] long long levlimopt(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
    [[maybe_unused]] void levlimopt_deinit(UDF_INIT *initid);
}

[[maybe_unused]]
int levlimopt_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, LEVLIMOPT_ARG_NUM_ERROR, LEVLIMOPT_ARG_NUM_ERROR_LEN);
        return 1;
    }
    // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != INT_RESULT) {
        strncpy(message, LEVLIMOPT_ARG_TYPE_ERROR, LEVLIMOPT_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to preallocate a buffer.
    initid->ptr = (char *)new(std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    if (initid->ptr == nullptr) {
        strncpy(message, LEVLIMOPT_MEM_ERROR, LEVLIMOPT_MEM_ERROR_LEN);
        return 1;
    }

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
void levlimopt_deinit(UDF_INIT *initid) {
    delete[] reinterpret_cast<int*>(initid->ptr);
}

[[maybe_unused]]
long long levlimopt(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {
#ifdef PRINT_DEBUG
    std::cout << "levlimopt" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[1];
#endif

    // Fetch preallocated buffer. The only difference between damlevmin and levlimopt is that damlevmin also persists
    // the max and updates it right before the final return statement.
    int *buffer   = reinterpret_cast<int *>(initid->ptr);
    long long max = std::min(*(reinterpret_cast<long long *>(args->args[2])), DAMLEV_MAX_EDIT_DIST);

    // Validate max distance and update.
    // This code is common to algorithms with limits.
#include "validate_max.h"

    // The pre-algorithm code is the same for all algorithm variants. It handles
    //     - basic setup & initialization
    //     - trimming of common prefix/suffix
#include "prealgorithm.h"

    // Lambda function for 2D matrix indexing in the 1D buffer
    auto idx = [m](int i, int j) { return i * (m + 1) + j; };
    int previous_minimum_within_row = 0;

    // Main loop to calculate the Damerau-Levenshtein distance
    for (int i = 1; i <= n; ++i) {
        // Initialize first column
        buffer[idx(i, 0)] = i;
        // Keep track of the minimum number of edits we have proven are necessary. If this
        // number ever exceeds `max`, we can bail.
        int minimum_within_row = std::max(i, m - n);

        // We only need to look in the window between i-max <= j <= i+max, because beyond
        // that window we would need (at least) another max inserts/deletions in the
        // "path" to arrive at the (n,m) cell.
        const int start_j = std::max(1, i - (effective_max-previous_minimum_within_row));
        const int end_j   = std::min(m, i + (effective_max-previous_minimum_within_row));
        // Assume anything outside the band contains more than max. The only cells outside the
        // band we actually look at are positions (i,start_j-1) and  (i,end_j+1), so we
        // pre-fill it with max + 1.
        if (start_j > 1) buffer[idx(i, start_j-1)] = max + 1;
        if (end_j   < m) buffer[idx(i, end_j+1)]   = max + 1;
#ifdef PRINT_DEBUG
        std::cout << start_j << "<= j <= " << end_j << "\n";
#endif

        for (int j = start_j; j <= end_j; ++j) {
            int cost          = (subject[i - 1] == query[j - 1]) ? 0 : 1;
            buffer[idx(i, j)] = std::min({buffer[idx(i - 1, j)] + 1,
                                          buffer[idx(i, j - 1)] + 1,
                                          buffer[idx(i - 1, j - 1)] + cost});

            minimum_within_row = std::min(minimum_within_row, buffer[idx(i, j)]);

#ifdef CAPTURE_METRICS
            metrics.cells_computed++;
#endif
        }

        // Early exit if the minimum edit distance exceeds the effective maximum
        if (minimum_within_row > static_cast<int>(effective_max)) {
#ifdef CAPTURE_METRICS
            metrics.early_exit++;
            metrics.algorithm_time += algorithm_timer.elapsed();
            metrics.total_time += call_timer.elapsed();
#endif
#ifdef PRINT_DEBUG
            std::cout << "Bailing early" << '\n';
            printMatrix(buffer, n, m, subject, query);
#endif
            return max + 1;
        }
        // previous_minimum_within_row = minimum_within_row;
    }

    // Return the final Damerau-Levenshtein distance
#ifdef PRINT_DEBUG
    printMatrix(buffer, n, m, subject, query);
#endif
#ifdef CAPTURE_METRICS
    metrics.algorithm_time += algorithm_timer.elapsed();
    metrics.total_time += call_timer.elapsed();
#endif
    return buffer[idx(n, m)];
}
