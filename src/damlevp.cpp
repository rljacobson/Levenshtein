/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    17 January 2019

    This implementation is better than most others out there. It is extremely
    fast and efficient.
            __—R.__

    <hr>
    `DAMLEVP()` computes the normalized Damarau Levenshtein edit distance between two strings.
    The normalization is the edit distance divided by the length of the longest string:
        ("edit distance")/("length of longest string").

    Syntax:

        DAMLEVP(String1, String2);

    `String1`:  A string constant or column.
    `String2`:  A string constant or column to be compared to `String1`.

    Returns: A floating point number equal to the normalized edit distance between `String1` and
    `String2`.

    Example Usage:

        SELECT Name, DAMLEVP(Name, "Vladimir Iosifovich Levenshtein") AS
            EditDist FROM CUSTOMERS WHERE DAMLEVP(Name, "Vladimir Iosifovich Levenshtein")  <= 0.2;

    The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
    where `Name` has edit distance within 20% of "Vladimir Iosifovich Levenshtein".

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
        DAMLEVP_ARG_NUM_ERROR[] = "Wrong number of arguments. DAMLEVP() requires two arguments:\n"
                                  "\t1. A string.\n"
                                  "\t2. Another string.";
constexpr const auto DAMLEVP_ARG_NUM_ERROR_LEN = std::size(DAMLEVP_ARG_NUM_ERROR) + 1;
constexpr const char DAMLEVP_MEM_ERROR[] = "Failed to allocate memory for DAMLEVP"
                                           " function.";
constexpr const auto DAMLEVP_MEM_ERROR_LEN = std::size(DAMLEVP_MEM_ERROR) + 1;
constexpr const char
        DAMLEVP_ARG_TYPE_ERROR[] = "Arguments have wrong type. DAMLEVP() requires two arguments:\n"
                                   "\t1. A string.\n"
                                   "\t2. Another string.";
constexpr const auto DAMLEVP_ARG_TYPE_ERROR_LEN = std::size(DAMLEVP_ARG_TYPE_ERROR) + 1;


UDF_SIGNATURES_TYPE(damlevp, double)


[[maybe_unused]]
int damlevp_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, DAMLEVP_ARG_NUM_ERROR, DAMLEVP_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments need to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != REAL_RESULT) {
        strncpy(message, DAMLEVP_ARG_TYPE_ERROR, DAMLEVP_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to preallocate a buffer.
    initid->ptr = (char *)new(std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    if (initid->ptr == nullptr) {
        strncpy(message, DAMLEVP_MEM_ERROR, DAMLEVP_MEM_ERROR_LEN);
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
void damlevp_deinit(UDF_INIT *initid) {
    delete[] reinterpret_cast<int*>(initid->ptr);
}

[[maybe_unused]]
double damlevp(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {

#ifdef PRINT_DEBUG
    std::cout << "damlevp" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[5];
#endif

    // Fetch preallocated buffer. The only difference between damlevmin and damlevp is that damlevmin also persists
    // the max and updates it right before the final return statement.
    int *buffer   = reinterpret_cast<int *>(initid->ptr);
    // Retrieve the similarity and compute max.
    double similarity = *(reinterpret_cast<double *>(args->args[7]));
    // The algorithm works with number of edits, a positive integer. For similarity, the
    // number of edits permitted depends on the length of the longest string.
    long long max = std::min(
            static_cast<long long>(
                (1.0-similarity)*std::max(args->lengths[0], args->lengths[1])
            ),
            DAMLEV_MAX_EDIT_DIST
            );

    // Validate max distance and update.
    // This code is common to algorithms with limits.
#include "validate_max.h"

    // The pre-algorithm code is the same for all algorithm variants. It handles
    //     - basic setup & initialization
    //     - trimming of common prefix/suffix
#include "prealgorithm.h"

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

    // We also use the following as the similarity analog of `max+1`. This is somewhat
    // arbitrary, but we need to be able to return a similarity smaller than the
    // minimum required similarity.
    double max_result = (1.0-static_cast<double>(max+1)/static_cast<double>(m));
    max_result = std::max(0.0, max_result); // Must be positive.

    // We keep track of only two rows for this algorithm. See below for details.
    int *current  = buffer;
    int *previous = buffer + m + 1;
    std::iota(previous, previous + m + 1, 0);

    int minimum_within_row     = 0;
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

        // We only need to look in the window between i-max <= j <= i+max, because beyond
        // that window we would need (at least) another max inserts/deletions in the
        // "path" to arrive at the (n,m) cell.
        const int start_j = std::max(1, i - effective_max);
        const int end_j   = std::min(m, i + effective_max);

        // Assume anything outside the band contains more than max. The only cells outside the
        // band we actually look at are positions (i,start_j-1) and  (i,end_j+1), so we
        // pre-fill it with max + 1.
        if (start_j > 1) current[start_j - 1] = max + 1;
        if (end_j   < m) current[end_j + 1]   = max + 1;
#ifdef PRINT_DEBUG
        // Print column header
        std::cout << subject[i - 1] << " " << i << " ";
        for(int k = 1; k <= start_j-1; k++) std::cout << ". ";
#endif
        // Keep track of the minimum number of edits we have proven are necessary. If this
        // number ever exceeds `max`, we can bail.
        minimum_within_row = i;

        for (int j = start_j; j <= end_j; ++j) {
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

            minimum_within_row     = std::min(minimum_within_row, current_cell);
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
        if (end_j < m) std::cout << max + 1;
        for(int k = end_j+2; k <= m; k++) std::cout << " .";
        std::cout << "\n";
#endif
        // Early exit if the minimum edit distance exceeds the effective maximum
        if (minimum_within_row > static_cast<int>(effective_max)) {
#ifdef CAPTURE_METRICS
            metrics.early_exit++;
            metrics.algorithm_time += algorithm_timer.elapsed();
            metrics.total_time += call_timer.elapsed();
#endif
#ifdef PRINT_DEBUG
            std::cout << "Bailing early" << '\n';
#endif
            return max_result;
        }
    }

    // Compute and return the final similarity score
    double result = (1.0-static_cast<double>(current_cell)/static_cast<double>(m));
    result = std::max(0.0, result);
#ifdef CAPTURE_METRICS
    metrics.algorithm_time += algorithm_timer.elapsed();
    metrics.total_time += call_timer.elapsed();
#endif
    return std::max(result, max_result);
}
