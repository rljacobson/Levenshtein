/*
Copyright (C) 2024 Robert Jacobson
Distributed under the MIT License. See License.txt for details.

`min_edit_dist()` computes the Levenshtein edit distance between two strings when the
edit distance is less than a given number.

Syntax:

    min_edit_dist(String1, String2, PosInt);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`PosInt`:   A positive integer. If the distance between `String1` and
            `String2` is greater than `PosInt`, `min_edit_dist()` will stop its
            computation at `PosInt` and return `PosInt`. Make `PosInt` as
            small as you can to improve speed and efficiency. For example,
            if you put `where min_edit_dist(...) < k` in a `where`-clause, make
            `PosInt` be `k`.

Returns: Either an integer equal to the edit distance between `String1` and `String2` or `k`,
whichever is smaller.

Example Usage:

    select Name, min_edit_dist(Name, "Vladimir Iosifovich Levenshtein", 6) as
        EditDist from Customers where min_edit_dist(Name, "Vladimir Iosifovich Levenshtein", 6) <= 6;

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
        MIN_EDIT_DIST_ARG_NUM_ERROR[] = "Wrong number of arguments. min_edit_dist() requires three arguments:\n"
                                    "\t1. A string\n"
                                    "\t2. A string\n"
                                    "\t3. A maximum distance (0 <= int < ${DAMLEV_MAX_EDIT_DIST}).";
constexpr const auto MIN_EDIT_DIST_ARG_NUM_ERROR_LEN = std::size(MIN_EDIT_DIST_ARG_NUM_ERROR) + 1;
constexpr const char MIN_EDIT_DIST_MEM_ERROR[] = "Failed to allocate memory for min_edit_dist"
                                             " function.";
constexpr const auto MIN_EDIT_DIST_MEM_ERROR_LEN = std::size(MIN_EDIT_DIST_MEM_ERROR) + 1;
constexpr const char
        MIN_EDIT_DIST_ARG_TYPE_ERROR[] = "Arguments have wrong type. min_edit_dist() requires three arguments:\n"
                                     "\t1. A string\n"
                                     "\t2. A string\n"
                                     "\t3. A maximum distance (0 <= int < ${DAMLEV_MAX_EDIT_DIST}).";
constexpr const auto MIN_EDIT_DIST_ARG_TYPE_ERROR_LEN = std::size(MIN_EDIT_DIST_ARG_TYPE_ERROR) + 1;


UDF_SIGNATURES(min_edit_dist)


struct MinEditDistPersistant {
    int max;
    int *buffer; // Takes ownership of this buffer

    MinEditDistPersistant(int max, int *buffer): max(max), buffer(buffer){}

    ~MinEditDistPersistant(){ delete this->buffer; }
};

[[maybe_unused]]
int min_edit_dist_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, MIN_EDIT_DIST_ARG_NUM_ERROR, MIN_EDIT_DIST_ARG_NUM_ERROR_LEN);
        return 1;
    }
    // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != INT_RESULT) {
        strncpy(message, MIN_EDIT_DIST_ARG_TYPE_ERROR, MIN_EDIT_DIST_ARG_TYPE_ERROR_LEN);
        return 1;
    }

// Initialize persistent data
    int* buffer = new (std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    MinEditDistPersistant *data = new (std::nothrow) MinEditDistPersistant(DAMLEV_MAX_EDIT_DIST, buffer);
    // If memory allocation failed
    if (!buffer || !data) {
        strncpy(message, MIN_EDIT_DIST_MEM_ERROR, MIN_EDIT_DIST_MEM_ERROR_LEN);
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
void min_edit_dist_deinit(UDF_INIT *initid) {
    // As `MinEditDistPersistant` owns its buffer, `~MinEditDistPersistant` handles buffer deallocation.
    delete reinterpret_cast<MinEditDistPersistant*>(initid->ptr);
}

[[maybe_unused]]
long long min_edit_dist(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {
#ifdef PRINT_DEBUG
    std::cout << "min_edit_dist" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[4];
#endif

    // Fetch persistent data
    MinEditDistPersistant *data = reinterpret_cast<MinEditDistPersistant *>(initid->ptr);
    // This line and the line updating `data->max` right before the final `return` statement are the only differences
    // between min_edit_dist and bounded_edit_dist.
    int max = std::min(
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

    const int m_n    = m-n; // We use this a lot.
    int current_cell = 0;

    // Keeping track of start and end is slightly faster than keeping track of
    // right/left band for reasons I don't understand.
    // first cell computed
    int start_j = 1;
    // last cell computed, +1 because we start at second row (index 1)
    int end_j   = std::min((max + m_n) / 2 + 1, m);

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

        // The order of these initializations is crucial. This comes first so the value in the buffer isn't overwritten.
        int previous_cell = buffer[start_j-1];

        // Initialize first column
        // if (start_j == 1) // This line seems to make no difference.
        buffer[0] = i;

        // Assume anything outside the band contains more than max. The only cells outside the
        // band we actually look at are positions (i,start_j-1) and  (i,end_j+1), so we
        // pre-fill it with max + 1.
        // if (start_j > 1) buffer[start_j-1] = max + 1;
        // if (end_j   < m) buffer[end_j+1]   = max + 1;

#ifdef PRINT_DEBUG
        // Print column header
        std::cout << subject[i - 1] << (i<10? "  ": " ") << i << " ";
        for(int k = 1; k <= start_j-2; k++) std::cout << " . ";
        if (start_j > 1) std::cout << (max < 9? " " : "")  << max + 1 << " ";
#endif

        // Main inner loop
        for (int j = start_j; j <= end_j; ++j) {
            int cost = (subject[i - 1] == query[j - 1]) ? 0 : 1;
            // See the declaration of `previous_cell` for an explanation of this.
            // `previous_cell` = matrix(i-1, j-1)
            //       cell[j-1] = matrix(i, j-1)
            //         cell[j] = matrix(i-1, j)
            current_cell = std::min({buffer[j]   + 1,
                                     buffer[j-1] + 1,
                                     previous_cell + cost});

#ifdef PRINT_DEBUG
            std::cout << (current_cell < 10 ? " ": "") << current_cell << " ";
#endif
#ifdef CAPTURE_METRICS
            metrics.cells_computed++;
#endif

            previous_cell = buffer[j];    // Save cell value for next iteration
            buffer[j]     = current_cell; // Overwrite
        }
#ifdef PRINT_DEBUG
        if (end_j < m) std::cout << (max < 9? " " : "") << max + 1 << " ";
        for(int k = end_j+2; k <= m; k++) std::cout << " . ";
        std::cout << "   " << start_j << " <= j <= " << end_j << "\n";
#endif

        // See if we can make the band narrower based on the row just computed.
        while(
            // end_j < m && // This should always be true
                end_j > 0
                && buffer[end_j] + std::abs(end_j - i - m_n) > max
                ) {
            end_j--;
        }
        // Increment for next row
        end_j = std::min(m, end_j + 1);

        // The starting point is a little different. It is "sticky" unless we
        // can prove it can shrink. Think of it as, both start and end do
        // the most conservative thing.
        while(
                start_j <= end_j
                && std::abs(i + m_n - start_j) + buffer[start_j] > max
                ) {
            start_j++;
        }

        if (end_j < start_j) {
#ifdef CAPTURE_METRICS
            metrics.early_exit++;
            metrics.algorithm_time += algorithm_timer.elapsed();
            metrics.total_time += call_timer.elapsed();
#endif
#ifdef PRINT_DEBUG
            std::cout << "EMPTY BAND: " << start_j << " <= j <= " << end_j << '\n';
#endif
            return max + 1;
        }
    }

    // This line and the line fetching `data->max` at the top of the function are the only differences
    // between min_edit_dist and bounded_edit_dist.
    data->max = std::min(current_cell, static_cast<int>(max));

    // Return the final Levenshtein distance
#ifdef CAPTURE_METRICS
    metrics.algorithm_time += algorithm_timer.elapsed();
    metrics.total_time += call_timer.elapsed();
#endif
    return std::min(max+1, current_cell);
}
