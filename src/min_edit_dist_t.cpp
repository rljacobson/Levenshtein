/*
Copyright (C) 2019 Robert Jacobson
Distributed under the MIT License. See License.txt for details.

`min_edit_dist_t(String1, String2, PosInt)`

Computes the Damarau-Levenshtein edit distance between two strings unless
that distance will exceed `current_min_distance`, the minimum edit distance
it has computed in the query so far, in which case it will return
`current_min_distance + 1`. The `current_min_distance` is initialized to
`PosInt`.

In the common case that we wish to find the rows that have the *smallest*
distance between strings, we can achieve significant performance
improvements if we stop the computation when we know the distance will be
*greater* than some other distance we have already computed.

Syntax:

    min_edit_dist_t(String1, String2, PosInt);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`PosInt`:   A positive integer. If the distance between `String1` and
            `String2` is greater than `PosInt`, `min_edit_dist_t()` will stop its
            computation at `PosInt` and return `PosInt`. Make `PosInt` as
            small as you can to improve speed and efficiency. For example,
            if you put `where min_edit_dist_t(...) <= k` in a `where`-clause, make
            `PosInt` be `k`.

Returns: Either an integer equal to the Damarau-Levenshtein edit distance between
`String1` and `String2`, if that distance is minimal among all distances computed
in the query, or some unspecified number greater than the minimum distance computed
in the query.

Example Usage:

    select Name, min_edit_dist_t(Name, "Vladimir Iosifovich Levenshtein", 6) as EditDist
         from Customers
         order by EditDist, Name asc;

The above will return all rows `(Name, EditDist)` from the `Customers` table.
The rows will be sorted in ascending order by `EditDist` and then by `Name`,
and the first row(s) will have `EditDist` equal to the edit distance between
`Name` and "Vladimir Iosifovich Levenshtein" or 6, whichever is smaller. All
other rows will have `EditDist` equal to some other unspecified larger number.

*/
#include "common.h"

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        MIN_EDIT_DIST_T_ARG_NUM_ERROR[] = "Wrong number of arguments. min_edit_dist_t() requires three arguments:\n"
                                    "\t1. A string\n"
                                    "\t2. A string\n"
                                    "\t3. A maximum distance (0 <= int < ${DAMLEV_MAX_EDIT_DIST}).";
constexpr const auto MIN_EDIT_DIST_T_ARG_NUM_ERROR_LEN = std::size(MIN_EDIT_DIST_T_ARG_NUM_ERROR) + 1;
constexpr const char MIN_EDIT_DIST_T_MEM_ERROR[] = "Failed to allocate memory for min_edit_dist_t"
                                             " function.";
constexpr const auto MIN_EDIT_DIST_T_MEM_ERROR_LEN = std::size(MIN_EDIT_DIST_T_MEM_ERROR) + 1;
constexpr const char
        MIN_EDIT_DIST_T_ARG_TYPE_ERROR[] = "Arguments have wrong type. min_edit_dist_t() requires three arguments:\n"
                                     "\t1. A string\n"
                                     "\t2. A string\n"
                                     "\t3. A maximum distance (0 <= int < ${DAMLEV_MAX_EDIT_DIST}).";
constexpr const auto MIN_EDIT_DIST_T_ARG_TYPE_ERROR_LEN = std::size(MIN_EDIT_DIST_T_ARG_TYPE_ERROR) + 1;


UDF_SIGNATURES(min_edit_dist_t)


struct MinEditDistTPersistant {
    int max;
    int *buffer; // Takes ownership of this buffer

    MinEditDistTPersistant(int max, int *buffer): max(max), buffer(buffer){}

    ~MinEditDistTPersistant(){ delete this->buffer; }
};

[[maybe_unused]]
int min_edit_dist_t_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, MIN_EDIT_DIST_T_ARG_NUM_ERROR, MIN_EDIT_DIST_T_ARG_NUM_ERROR_LEN);
        return 1;
    }
    // The arguments need to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != INT_RESULT) {
        strncpy(message, MIN_EDIT_DIST_T_ARG_TYPE_ERROR, MIN_EDIT_DIST_T_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Initialize persistent data
    int* buffer = new (std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    MinEditDistTPersistant *data = new (std::nothrow) MinEditDistTPersistant(DAMLEV_MAX_EDIT_DIST, buffer);
    // If memory allocation failed
    if (!buffer || !data) {
        strncpy(message, MIN_EDIT_DIST_T_MEM_ERROR, MIN_EDIT_DIST_T_MEM_ERROR_LEN);
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
void min_edit_dist_t_deinit(UDF_INIT *initid) {
    // As `MinEditDistTPersistant` owns its buffer, `~MinEditDistTPersistant` handles buffer deallocation.
    delete reinterpret_cast<MinEditDistTPersistant*>(initid->ptr);
}

[[maybe_unused]]
long long min_edit_dist_t(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {

#ifdef PRINT_DEBUG
    std::cout << "min_edit_dist_t" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[5];
#endif

    // Fetch persistent data
    MinEditDistTPersistant *data = reinterpret_cast<MinEditDistTPersistant *>(initid->ptr);
    // This line and the line updating `data->max` right before the final `return` statement are the only differences
    // between min_edit_dist_t and bounded_edit_dist_t.
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
    // a buffer of size 2*(m+1). Because of trimming, m may be smaller than the length
    // of the longest string.
    if( 2*(m+1) > DAMLEV_BUFFER_SIZE ) {
#ifdef CAPTURE_METRICS
        metrics.buffer_exceeded++;
        metrics.total_time += call_timer.elapsed();
#endif
        return 0;
    }

    const int m_n = m-n; // We use this a lot.

    // We keep track of only two rows for this algorithm. See below for details.
    int *current  = buffer;
    int *previous = buffer + m + 1;
    std::iota(previous, previous + m + 1, 0);

    int previous_previous_cell = 0;
    int previous_cell          = 0;
    int current_cell           = 0;

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

    // Main loop to calculate the Damerau-Levenshtein distance
    for (int i = 1; i <= n; ++i) {
        // The order of these initializations is crucial. This comes first so the value in the buffer isn't overwritten.
        previous_cell = current[start_j-1]; // = matrix(i-1, 0)

        // Initialize first column
        // if (start_j == 1) // This line seems to make no difference.
        current[0] = i;

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

            if(j>1){
                previous[j-2]      = previous_previous_cell;
            }
            previous_previous_cell = previous_cell;
            previous_cell          = current[j];
            current[j]             = current_cell;

#ifdef PRINT_DEBUG
            std::cout << (current_cell < 10 ? " ": "") << current_cell << " ";
#endif
#ifdef CAPTURE_METRICS
            metrics.cells_computed++;
#endif
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
                // Have to subtract one in the following as next character might make transposition
                && buffer[end_j] + std::abs(end_j - i - m_n) - 1 > max
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
                // Have to subtract one in the following as next character might make transposition
                && std::abs(i + m_n - start_j) + buffer[start_j] - 1 > max
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
    // between min_edit_dist_t and bounded_edit_dist_t.
    data->max = std::min(current_cell, static_cast<int>(max));

    // Return the final Damerau-Levenshtein distance
#ifdef CAPTURE_METRICS
    metrics.algorithm_time += algorithm_timer.elapsed();
    metrics.total_time += call_timer.elapsed();
#endif
    return std::min(max + 1, current_cell);
}
