/*
Copyright (C) 2019 Robert Jacobson
Distributed under the MIT License. See License.txt for details.

`min_similarity_t(String1, String2, RealNumber)`

Computes a similarity score in the range [0.0, 1.0] of two strings
unless that similarity score will be less than `current_max_similarity`,
the largest similarity score it has computed in the query so far, in
which case it will return some smaller value. The `current_max_similarity`
is initialized to `RealNumber`.

In the common case that we wish to find the rows that have the *greatest*
similarity between strings, we can achieve *significant* performance
improvements if we stop the computation when we know the similarity will be
*smaller* than some other similarity we have already computed. Under reasonable
conditions, the speed of this function (excluding overhead) can be only twice
as slow as doing nothing at all!

Similarity is computed from Damarau-Levenshtein edit distance by "normalizing"
using the length of the longest string:
    similarity = 1.0 - EditDistance(String1, String2)/max(len(String1), len(String2))

Syntax:

    min_similarity_t(String1, String2, RealNumber);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.
`PosInt`:   A positive integer. If the similarity between `String1` and
            `String2` is less than `RealNumber`, `min_similarity_t()` will stop its
            computation at `RealNumber` and return a number smaller than
            `RealNumber`. Make `RealNumber` as large as you can to improve
            speed and efficiency. For example, if you put
            `where min_similarity_t(...) >= p` in a `where`-clause, make
            `RealNumber` be `p`.

Returns: Either a real number equal to the similarity between `String1` and `String2`,
if that distance is minimal among all distances computed in the query, or some
unspecified number smaller than the minimum distance computed in the query.

Example Usage:

    select Name, min_similarity_t(Name, "Vladimir Iosifovich Levenshtein", 0.95) as Similarity
         from Customers
         order by Similarity, Name desc;

The above will return all rows `(Name, Similarity)` from the `Customers` table.
The rows will be sorted in descending order by `Similarity` and then by `Name`,
and the first row(s) will have `Similarity` equal to the similarity between
`Name` and "Vladimir Iosifovich Levenshtein" or 0.95, whichever is larger. All
other rows will have `EditDist` equal to some other unspecified smaller number.

*/
#include "common.h"

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        MIN_SIMILARITY_T_ARG_NUM_ERROR[] = "Wrong number of arguments. min_similarity_t() requires three arguments:\n"
                                  "\t1. A string.\n"
                                  "\t2. Another string.\n"
                                  "\t3. A real number.";
constexpr const auto MIN_SIMILARITY_T_ARG_NUM_ERROR_LEN = std::size(MIN_SIMILARITY_T_ARG_NUM_ERROR) + 1;
constexpr const char MIN_SIMILARITY_T_MEM_ERROR[] = "Failed to allocate memory for min_similarity_t"
                                           " function.";
constexpr const auto MIN_SIMILARITY_T_MEM_ERROR_LEN = std::size(MIN_SIMILARITY_T_MEM_ERROR) + 1;
constexpr const char
        MIN_SIMILARITY_T_ARG_TYPE_ERROR[] = "Arguments have wrong type. min_similarity_t() requires three arguments:\n"
                                   "\t1. A string.\n"
                                   "\t2. Another string.\n"
                                   "\t3. A real number.";
constexpr const auto MIN_SIMILARITY_T_ARG_TYPE_ERROR_LEN = std::size(MIN_SIMILARITY_T_ARG_TYPE_ERROR) + 1;


UDF_SIGNATURES_TYPE(min_similarity_t, double)


struct MinSimilarityTPersistant {
    double p;    // Only compute similarities that are at least p
    int *buffer; // Takes ownership of this buffer

    MinSimilarityTPersistant(double similarity, int *buffer): p(similarity), buffer(buffer){}

    ~MinSimilarityTPersistant(){ delete this->buffer; }
};

/// Converts minimum allowed similarity to maximum allowed number of edits for a given string length.
/// Assumes similarity is in the interval [0.0, 1.0].
inline constexpr long long similarity_to_max_edits(double similarity, int length) {
    return static_cast<int>((1.0 - similarity) * static_cast<double>(length));
}
/// The inverse of the above. Converts number of edits for a given string length to a similarity score.
/// Guaranteed to return number in the interval [0.0, 1.0] for edits <= length.
inline constexpr double edits_to_similarity(int edits, int length) {
    return (1.0 - static_cast<double>(edits)/static_cast<double>(length));
}

[[maybe_unused]]
int min_similarity_t_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, MIN_SIMILARITY_T_ARG_NUM_ERROR, MIN_SIMILARITY_T_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments need to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != REAL_RESULT) {
        strncpy(message, MIN_SIMILARITY_T_ARG_TYPE_ERROR, MIN_SIMILARITY_T_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Initialize persistent data
    int* buffer = new (std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    MinSimilarityTPersistant *data = new (std::nothrow) MinSimilarityTPersistant(0.0, buffer);
    // If memory allocation failed
    if (!buffer || !data) {
        strncpy(message, MIN_SIMILARITY_T_MEM_ERROR, MIN_SIMILARITY_T_MEM_ERROR_LEN);
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
void min_similarity_t_deinit(UDF_INIT *initid) {
    // As `MinSimilarityTPersistant` owns its buffer, `~MinSimilarityTPersistant` handles buffer deallocation.
    delete reinterpret_cast<MinSimilarityTPersistant*>(initid->ptr);
}

[[maybe_unused]]
double min_similarity_t(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {

#ifdef PRINT_DEBUG
    std::cout << "min_similarity_t" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[9];
#endif

    // Fetch persistent data
    MinSimilarityTPersistant *data = reinterpret_cast<MinSimilarityTPersistant *>(initid->ptr);

    // Retrieve the similarity and compute max.
    double similarity = data->p;

#include "validate_similarity.h"

    // The algorithm works with number of edits, a positive integer. For similarity, the
    // number of edits permitted depends on the length of the longest string.
    int max = static_cast<int>(
                similarity_to_max_edits(
                        similarity,
                        std::max(args->lengths[0], args->lengths[1])
                )
            );
    int *buffer = data->buffer;

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
                && current[end_j] + std::abs(end_j - i - m_n) - 1 > max
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
                && std::abs(i + m_n - start_j) + current[start_j] - 1 > max
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
            return max_result;
        }
    }

    // Compute and return the final similarity score
    double result = (1.0-static_cast<double>(current_cell)/static_cast<double>(m));
    result = std::max(0.0, result);
    data->p = std::max(similarity, result);
#ifdef CAPTURE_METRICS
    metrics.algorithm_time += algorithm_timer.elapsed();
    metrics.total_time += call_timer.elapsed();
#endif
    return std::max(result, max_result);
}
