/*
Copyright (C) 2024 Robert Jacobson
Distributed under the MIT License. See License.txt for details.

`edit_dist()` computes the Levenshtein edit distance between two strings when the
edit distance is less than a given number.

Syntax:

    edit_dist(String1, String2);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.

Returns: The edit distance between `String1` and `String2`.

Example Usage:

    select Name, edit_dist(Name, "Vladimir Iosifovich Levenshtein") AS
        EditDist from Customers where  edit_dist(Name, "Vladimir Iosifovich Levenshtein") <= 6;

The above will return all rows `(Name, EditDist)` from the `Customers` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".
*/
#include "common.h"

#ifdef PRINT_DEBUG
void printMatrix(const int* dp, int n, int m, const std::string_view& S1, const std::string_view& S2);
#endif


// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        EDIT_DIST_ARG_NUM_ERROR[] = "Wrong number of arguments. edit_dist() requires two arguments:\n"
                                    "\t1. A string\n"
                                    "\t2. A string";
constexpr const auto EDIT_DIST_ARG_NUM_ERROR_LEN = std::size(EDIT_DIST_ARG_NUM_ERROR) + 1;
constexpr const char EDIT_DIST_MEM_ERROR[] = "Failed to allocate memory for edit_dist"
                                             " function.";
constexpr const auto EDIT_DIST_MEM_ERROR_LEN = std::size(EDIT_DIST_MEM_ERROR) + 1;
constexpr const char
        EDIT_DIST_ARG_TYPE_ERROR[] = "Arguments have wrong type. edit_dist() requires two arguments:\n"
                                     "\t1. A string\n"
                                     "\t2. A string";
constexpr const auto EDIT_DIST_ARG_TYPE_ERROR_LEN = std::size(EDIT_DIST_ARG_TYPE_ERROR) + 1;


UDF_SIGNATURES(edit_dist)


[[maybe_unused]]
int edit_dist_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 2 arguments:
    if (args->arg_count != 2) {
        strncpy(message, EDIT_DIST_ARG_NUM_ERROR, EDIT_DIST_ARG_NUM_ERROR_LEN);
        return 1;
    }
    // The arguments need to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT) {
        strncpy(message, EDIT_DIST_ARG_TYPE_ERROR, EDIT_DIST_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to preallocate a buffer.
    initid->ptr = (char *)new(std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    if (initid->ptr == nullptr) {
        strncpy(message, EDIT_DIST_MEM_ERROR, EDIT_DIST_MEM_ERROR_LEN);
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
void edit_dist_deinit(UDF_INIT *initid) {
    delete[] reinterpret_cast<int*>(initid->ptr);
}

[[maybe_unused]]
long long edit_dist(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {
#ifdef PRINT_DEBUG
    std::cout << "edit_dist" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[2];
#endif

    // Fetch preallocated buffer. The only difference between min_edit_dist and lev is that min_edit_dist also persists
    // the max and updates it right before the final return statement.
    int *buffer   = reinterpret_cast<int *>(initid->ptr);

    // The pre-algorithm code is the same for all algorithm variants. It handles
    //     - basic setup & initialization
    //     - trimming of common prefix/suffix
#define SUPPRESS_MAX_CHECK
#include "prealgorithm.h"
#undef SUPPRESS_MAX_CHECK

    // Check if buffer size required exceeds available buffer size. This algorithm needs
    // a buffer of size (m+1). Because of trimming, this may be smaller than the length
    // of the longest string.
    if( m+1 > DAMLEV_BUFFER_SIZE ) {
#ifdef CAPTURE_METRICS
        metrics.buffer_exceeded++;
        metrics.total_time += call_timer.elapsed();
#endif
        return 0;
    }

    int current_cell = 0;

#ifdef PRINT_DEBUG
    // Print the matrix header
    std::cout << "     ";
    for(int k = 0; k < m; k++) std::cout << " " << query[k] << " ";
    std::cout << "\n  ";
    for(int k = 0; k <= m; k++) std::cout << (k < 10? " ": "") << k << " ";
    std::cout << "\n";
#endif

    // Main loop to calculate the Levenshtein distance
    for (int i = 1; i <= n; ++i) {


        // We use only a single "row" instead of a full matrix. Let's call it cell[*].
        // The current cell, cell[j], is matrix position (row, col) = (i, j).  To compute
        // this cell, we need matrix(i-1, j), matrix(i, j-1), and matrix(i-1, j-1). When
        // computing the current cell value,
        //               (  matrix(i, p)    for p < j
        //     cell[p] = |
        //               (  matrix(i-1, p)   for p >= j.
        // Thus, before we overwrite cell[j], it contains matrix(i-1, j). In the previous
        // iteration, we overwrote matrix(i-1, j-1) with the value of matrix(i, j-1). Thus,
        // we need to keep the value in cell[j] before overwriting it for use in the next
        // iteration. We store it in the variable `previous_cell`. The invariant is that
        // `previous_cell` = matrix(i-1, j-1).
        int previous_cell = i-1; // = buffer[start_j-1];

        // Initialize first column
        buffer[0] = i;

#ifdef PRINT_DEBUG
        // Print column header
        std::cout << subject[i - 1] << (i<10? "  ": " ") << i << " ";
        // for(int k = 1; k <= start_j-2; k++) std::cout << " . ";
        // if (start_j > 1) std::cout << (max < 9? " " : "")  << max + 1 << " ";
#endif

        // Main inner loop
        for (int j = 1; j <= m; ++j) {
            int cost          = (subject[i - 1] == query[j - 1]) ? 0 : 1;
            // See the declaration of `previous_cell` for an explanation of this.
            // `previous_cell` = matrix(i-1, j-1)
            //       cell[j-1] = matrix(i, j-1)
            //         cell[j] = matrix(i-1, j)
            current_cell = std::min({buffer[j] + 1,
                                     buffer[j - 1] + 1,
                                     previous_cell + cost});

            previous_cell      = buffer[j];    // Save cell value for next iteration
            buffer[j]          = current_cell; // Overwrite

#ifdef PRINT_DEBUG
            std::cout << (current_cell < 10 ? " ": "") << current_cell << " ";
#endif
#ifdef CAPTURE_METRICS
            metrics.cells_computed++;
#endif
        }

#ifdef PRINT_DEBUG
        // if (end_j < m) std::cout << (max < 9? " " : "") << max + 1 << " ";
        // for(int k = end_j+2; k <= m; k++) std::cout << " . ";
        // std::cout << "   " << start_j << " <= j <= " << end_j << "\n";
        std::cout << "\n";
#endif

    }

    // Return the final Levenshtein distance
#ifdef CAPTURE_METRICS
    metrics.algorithm_time += algorithm_timer.elapsed();
    metrics.total_time += call_timer.elapsed();
#endif
    return static_cast<long long>(current_cell);
}
