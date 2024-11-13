/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    17 January 2019

    This implementation is better than most others out there. It is extremely
    fast and efficient.
            __—R.__

    <hr>
    `DAMLEV()` computes the Damarau Levenshtein edit distance between two strings when the
    edit distance is less than a given number.

    Syntax:

        DAMLEV(String1, String2, PosInt);

    `String1`:  A string constant or column.
    `String2`:  A string constant or column to be compared to `String1`.
    `PosInt`:   A positive integer. If the distance between `String1` and
                `String2` is greater than `PosInt`, `DAMLEV()` will stop its
                computation at `PosInt` and return `PosInt`. Make `PosInt` as
                small as you can to improve speed and efficiency. For example,
                if you put `WHERE DAMLEV(...) < k` in a `WHERE`-clause, make
                `PosInt` be `k`.

    Returns: Either an integer equal to the edit distance between `String1` and `String2` or `k`,
    whichever is smaller.

    Example Usage:

        SELECT Name, DAMLEV(Name, "Vladimir Iosifovich Levenshtein", 6) AS
            EditDist FROM CUSTOMERS WHERE  DAMLEV(Name, "Vladimir Iosifovich Levenshtein", 6) <= 6;

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

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        DAMLEV_ARG_NUM_ERROR[] = "Wrong number of arguments. DAMLEV() requires two arguments:\n"
                                    "\t1. A string\n"
                                    "\t2. A string";
constexpr const auto DAMLEV_ARG_NUM_ERROR_LEN = std::size(DAMLEV_ARG_NUM_ERROR) + 1;
constexpr const char DAMLEV_MEM_ERROR[] = "Failed to allocate memory for DAMLEV"
                                             " function.";
constexpr const auto DAMLEV_MEM_ERROR_LEN = std::size(DAMLEV_MEM_ERROR) + 1;
constexpr const char
        DAMLEV_ARG_TYPE_ERROR[] = "Arguments have wrong type. DAMLEV() requires two arguments:\n"
                                     "\t1. A string\n"
                                     "\t2. A string";
constexpr const auto DAMLEV_ARG_TYPE_ERROR_LEN = std::size(DAMLEV_ARG_TYPE_ERROR) + 1;

// Use a "C" calling convention.
extern "C" {
    [[maybe_unused]] int damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    [[maybe_unused]] long long damlev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
    [[maybe_unused]] void damlev_deinit(UDF_INIT *initid);
}

[[maybe_unused]]
int damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 2 arguments:
    if (args->arg_count != 2) {
        strncpy(message, DAMLEV_ARG_NUM_ERROR, DAMLEV_ARG_NUM_ERROR_LEN);
        return 1;
    }
    // The arguments need to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT) {
        strncpy(message, DAMLEV_ARG_TYPE_ERROR, DAMLEV_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to preallocate a buffer.
    initid->ptr = (char *)new(std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    if (initid->ptr == nullptr) {
        strncpy(message, DAMLEV_MEM_ERROR, DAMLEV_MEM_ERROR_LEN);
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
void damlev_deinit(UDF_INIT *initid) {
    delete[] reinterpret_cast<int*>(initid->ptr);
}

[[maybe_unused]]
long long damlev(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {

#ifdef PRINT_DEBUG
    std::cout << "damlev" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[3];
#endif

    // Fetch preallocated buffer.
    int *buffer   = reinterpret_cast<int *>(initid->ptr);

    // The pre-algorithm code is the same for all algorithm variants. It handles
    //     - basic setup & initialization
    //     - trimming of common prefix/suffix
#define SUPPRESS_MAX_CHECK
#include "prealgorithm.h"
#undef SUPPRESS_MAX_CHECK

    // Check if buffer size required exceeds available buffer size. This algorithm needs
    // a buffer of size 2*(m+1). Because of trimming, m may be smaller than the length
    // of the longest string.
    if( 2*(m+1) > DAMLEV_BUFFER_SIZE ) {
#ifdef CAPTURE_METRICS
        metrics.buffer_exceeded++;
        metrics.total_time += call_timer.elapsed();
#endif
        return 0;
    }

    // We keep track of only two rows for this algorithm. See below for details.
    int *current  = buffer;
    int *previous = buffer + m + 1;
    std::iota(previous, previous + m + 1, 0);

    // int minimum_within_row     = 0;
    int previous_cell          = 0;
    int previous_previous_cell = 0;
    int current_cell           = 0;

#ifdef PRINT_DEBUG
    // Print the matrix header
    std::cout << "    ";
    for(int k = 0; k < m; k++) std::cout << query[k] << " ";
    std::cout << "\n  ";
    for(int k = 0; k < m+1; k++) std::cout << k << " ";
    std::cout << "\n";
#endif

    // Main loop to calculate the Damerau-Levenshtein distance
    for (int i = 1; i <= n; ++i) {
        // Initialize first column
        current[0] = i;
        previous_cell = i-1; // = matrix(i-1, 0)

#ifdef PRINT_DEBUG
        // Print column header
        std::cout << subject[i - 1] << " " << i << " ";
        // for(int k = 1; k <= start_j-2; k++) std::cout << ". ";
        // if (start_j > 1) std::cout << max + 1 << " ";
#endif
        // Keep track of the minimum number of edits we have proven are necessary. If this
        // number ever exceeds `max`, we can bail.
        // minimum_within_row = i;

        for (int j = 1; j <= m; ++j) {
            /*
            At the start of the computation of matrix(i, j) we have:
                current[p]  = {  matrix(i, p)     for p < j
                              {  matrix(i-1, p)   for p >= j
                previous[p] = {  matrix(i-1, p)   for p < j - 2
                              {  matrix(i-2, p)   for p >= j - 2
                previous_cell          = matrix(i-1, j-1)
                previous_previous_cell = matrix(i-1, j-2)

            To compute matrix(i, j), we need to use the following:
                matrix(i  , j-1) = current[j-1]
                matrix(i-2, j-2) = previous[j-2]
                matrix(i-1, j-1) = previous_cell
                matrix(i-1, j  ) = current[j]
            */

            int cost     = (subject[i - 1] == query[j - 1]) ? 0 : 1;
            current_cell = std::min({current[j]    + 1,
                                     current[j-1]  + 1,
                                     previous_cell + cost});

            // Check for transpositions
            if (i > 1 && j > 1 && subject[i - 1] == query[j - 2] && subject[i - 2] == query[j - 1]) {
                current_cell = std::min(current_cell, previous[j-2] + cost);
            }

            /*
            We compute current_cell = matrix(i, j) and then perform the update for the next iteration:
                previous[j-2]          <-- previous_previous_cell := matrix(i-1, j-2)
                previous_previous_cell <-- previous_cell          := matrix(i-1, j-1)
                previous_cell          <-- current[j]             := matrix(i-1, j)
                current[j]             <-- current_cell           := matrix(i, j)
            */

            // minimum_within_row     = std::min(minimum_within_row, current_cell);
            if(j>1){
                previous[j-2]      = previous_previous_cell;
            }
            previous_previous_cell = previous_cell;
            previous_cell          = current[j];
            current[j]             = current_cell;

#ifdef PRINT_DEBUG
            std::cout << current_cell << " ";
#endif
#ifdef CAPTURE_METRICS
            metrics.cells_computed++;
#endif
        }

#ifdef PRINT_DEBUG
        // if (end_j < m) std::cout << max + 1 << " ";
        // for(int k = end_j+2; k <= m; k++) std::cout << ". ";
        // std::cout << "   " << start_j << " <= j <= " << end_j << "\n";
        std::cout << '\n';
#endif
    }

    // Return the final Damerau-Levenshtein distance
#ifdef CAPTURE_METRICS
    metrics.algorithm_time += algorithm_timer.elapsed();
    metrics.total_time += call_timer.elapsed();
#endif
    return static_cast<long long>(current_cell);
}
