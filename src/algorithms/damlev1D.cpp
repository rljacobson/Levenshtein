// damlev1D.cpp
#include <algorithm>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <string_view>
#include "common.h"

// Error messages.
constexpr const char
        DAMLEVMIN_ARG_NUM_ERROR[] = "Wrong number of arguments. damlev1D() requires three arguments:\n"
                                    "\t1. A string\n"
                                    "\t2. A string\n"
                                    "\t3. A maximum distance (0 <= int < DAMLEV_MAX_EDIT_DIST).";
constexpr const auto DAMLEVMIN_ARG_NUM_ERROR_LEN = std::size(DAMLEVMIN_ARG_NUM_ERROR);

constexpr const char DAMLEVMIN_MEM_ERROR[] = "Failed to allocate memory for damlev1D function.";
constexpr const auto DAMLEVMIN_MEM_ERROR_LEN = std::size(DAMLEVMIN_MEM_ERROR);

constexpr const char
        DAMLEVMIN_ARG_TYPE_ERROR[] = "Arguments have wrong type. damlev1D() requires three arguments:\n"
                                     "\t1. A string\n"
                                     "\t2. A string\n"
                                     "\t3. A maximum distance (0 <= int < DAMLEV_MAX_EDIT_DIST).";
constexpr const auto DAMLEVMIN_ARG_TYPE_ERROR_LEN = std::size(DAMLEVMIN_ARG_TYPE_ERROR);

// Maximum allowed edit distance.
// Adjust this value as needed.
//constexpr int DAMLEV_MAX_EDIT_DIST = 100;

// Use a "C" calling convention for MySQL UDF functions.
extern "C" {
[[maybe_unused]] int damlev1D_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
[[maybe_unused]] long long damlev1D(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
[[maybe_unused]] void damlev1D_deinit(UDF_INIT *initid);
}

struct DamLevMinPersistant {
    int max;
    int* buffer; // 1D buffer for dynamic programming
    std::unordered_map<char, int> last_row_id; // Tracks last occurrence of characters

    DamLevMinPersistant(int max_distance, int* buf)
            : max(max_distance), buffer(buf), last_row_id() {}

    ~DamLevMinPersistant() { delete[] buffer; }
};

[[maybe_unused]]
int damlev1D_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, DAMLEVMIN_ARG_NUM_ERROR, DAMLEVMIN_ARG_NUM_ERROR_LEN);
        message[DAMLEVMIN_ARG_NUM_ERROR_LEN - 1] = '\0'; // Ensure null-termination
        return 1;
    }

    // Argument types: string, string, int
    if (args->arg_type[0] != STRING_RESULT ||
        args->arg_type[1] != STRING_RESULT ||
        args->arg_type[2] != INT_RESULT) {
        strncpy(message, DAMLEVMIN_ARG_TYPE_ERROR, DAMLEVMIN_ARG_TYPE_ERROR_LEN);
        message[DAMLEVMIN_ARG_TYPE_ERROR_LEN - 1] = '\0'; // Ensure null-termination
        return 1;
    }

    // Initialize persistent data
    // Allocate buffer for dynamic programming.
    // Size depends on DAMLEV_MAX_EDIT_DIST to ensure linear space.
    int* buffer = new (std::nothrow) int[(DAMLEV_MAX_EDIT_DIST + 1)];
    DamLevMinPersistant* data = new (std::nothrow) DamLevMinPersistant(DAMLEV_MAX_EDIT_DIST, buffer);

    // If memory allocation failed
    if (!buffer || !data) {
        strncpy(message, DAMLEVMIN_MEM_ERROR, DAMLEVMIN_MEM_ERROR_LEN);
        message[DAMLEVMIN_MEM_ERROR_LEN - 1] = '\0'; // Ensure null-termination
        delete[] buffer;
        delete data;
        return 1;
    }

    initid->ptr = reinterpret_cast<char*>(data);

    // Set initid properties
#if defined(RETURN_NULL_ON_BAD_MAX) || defined(RETURN_NULL_ON_BUFFER_EXCEEDED)
    initid->maybe_null = 1;
#else
    initid->maybe_null = 0;
#endif

    // Initialize max to DAMLEV_MAX_EDIT_DIST
    data->max = DAMLEV_MAX_EDIT_DIST;

    return 0;
}

[[maybe_unused]]
void damlev1D_deinit(UDF_INIT *initid) {
    // Fetch the persistent data
    DamLevMinPersistant* data = reinterpret_cast<DamLevMinPersistant*>(initid->ptr);

    // Delete the persistent data, which also deallocates the buffer
    delete data;
}

[[maybe_unused]]
long long damlev1D(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {
    // Fetch persistent data
    DamLevMinPersistant* data = reinterpret_cast<DamLevMinPersistant*>(initid->ptr);
    if (!data) {
        strncpy(error, DAMLEVMIN_MEM_ERROR, DAMLEVMIN_MEM_ERROR_LEN);
        error[DAMLEVMIN_MEM_ERROR_LEN - 1] = '\0'; // Ensure null-termination
        return 0;
    }

    // Retrieve max distance from arguments
    long long user_max = *((long long*)args->args[2]);
    if (user_max < 0 || user_max >= DAMLEV_MAX_EDIT_DIST) {
        strncpy(error, DAMLEVMIN_ARG_TYPE_ERROR, DAMLEVMIN_ARG_TYPE_ERROR_LEN);
        error[DAMLEVMIN_ARG_TYPE_ERROR_LEN - 1] = '\0'; // Ensure null-termination
#if defined(RETURN_NULL_ON_BAD_MAX)
        *is_null = 1;
        return 0; // Or an appropriate null value
#else
        return DAMLEV_MAX_EDIT_DIST + 1;
#endif
    }

    // Update the effective max distance
    int effective_max = std::min(static_cast<int>(user_max), data->max);
    data->max = effective_max;

    // Retrieve and validate input strings
    if (args->args[0] == nullptr || args->lengths[0] == 0 ||
        args->args[1] == nullptr || args->lengths[1] == 0) {
        // If one of the strings is null or empty, return the length of the other string
        if (args->args[0] == nullptr || args->lengths[0] == 0)
            return args->lengths[1];
        else
            return args->lengths[0];
    }

    // Define string views for subject and query
    std::string_view subject_view{args->args[0], static_cast<size_t>(args->lengths[0])};
    std::string_view query_view{args->args[1], static_cast<size_t>(args->lengths[1])};

    // Trim common prefix
    size_t start_offset = 0;
    while (start_offset < subject_view.size() && start_offset < query_view.size() &&
           subject_view[start_offset] == query_view[start_offset]) {
        start_offset++;
    }

    // If one string is a prefix of the other
    if (start_offset == subject_view.size())
        return static_cast<long long>(query_view.size() - start_offset);
    if (start_offset == query_view.size())
        return static_cast<long long>(subject_view.size() - start_offset);

    // Trim common suffix
    size_t end_offset = 0;
    while (end_offset < (subject_view.size() - start_offset) &&
           end_offset < (query_view.size() - start_offset) &&
           subject_view[subject_view.size() - 1 - end_offset] == query_view[query_view.size() - 1 - end_offset]) {
        end_offset++;
    }

    // Extract the trimmed strings
    subject_view = subject_view.substr(start_offset, subject_view.size() - start_offset - end_offset);
    query_view = query_view.substr(start_offset, query_view.size() - start_offset - end_offset);

    // If after trimming, one of the strings is empty
    if (subject_view.empty())
        return static_cast<long long>(query_view.size());
    if (query_view.empty())
        return static_cast<long long>(subject_view.size());

    // Reorder to ensure subject is the smaller string
    if (query_view.size() < subject_view.size()) {
        std::swap(subject_view, query_view);
    }

    // Initialize buffer
    // Buffer size is the length of the smaller string + 1
    size_t m = query_view.size();
    size_t n = subject_view.size();

    if (m + 1 > static_cast<size_t>(DAMLEV_MAX_EDIT_DIST + 1)) {
        strncpy(error, DAMLEVMIN_MEM_ERROR, DAMLEVMIN_MEM_ERROR_LEN);
        error[DAMLEVMIN_MEM_ERROR_LEN - 1] = '\0'; // Ensure null-termination
#if defined(RETURN_NULL_ON_BUFFER_EXCEEDED)
        *is_null = 1;
        return 0; // Or an appropriate null value
#else
        return DAMLEV_MAX_EDIT_DIST + 1;
#endif
    }

    // Initialize the buffer with initial row values
    for (size_t j = 0; j <= m; ++j) {
        data->buffer[j] = static_cast<int>(j);
    }

    // Initialize variables for the algorithm
    std::unordered_map<char, int>& last_row_id = data->last_row_id;
    int last_i2l1 = 0; // H_i-2,l-1

    // Iterate through each character of the subject string
    for (int i = 1; i <= static_cast<int>(n); ++i) {
        int column_min = std::numeric_limits<int>::max();
        int prev_diag = data->buffer[0]; // H_i-1,j-1
        data->buffer[0] = i;

        for (int j = 1; j <= static_cast<int>(m); ++j) {
            int cost = (subject_view[i - 1] == query_view[j - 1]) ? 0 : 1;

            // Calculate the minimum of deletion, insertion, and substitution
            int current = std::min({
                                           data->buffer[j] + 1,           // Deletion (up)
                                           data->buffer[j - 1] + 1,       // Insertion (left)
                                           prev_diag + cost               // Substitution (diag)
                                   });

            // Check for transpositions
            if (i > 1 && j > 1 &&
                subject_view[i - 1] == query_view[j - 2] &&
                subject_view[i - 2] == query_view[j - 1]) {
                current = std::min(current, last_i2l1 + cost); // Transposition
            }

            // Update column_min for early termination
            if (current < column_min) {
                column_min = current;
            }

            // Update last_i2l1 for next iteration
            last_i2l1 = data->buffer[j];

            // Set the current cell
            data->buffer[j] = current;

            // Update the previous diagonal value
            prev_diag = last_i2l1;
        }

        // Early termination if the minimum edit distance exceeds the effective max
        if (column_min > effective_max) {
            return static_cast<long long>(user_max + 1);
        }
    }

    // The final edit distance is in buffer[m]
    int distance = data->buffer[m];

    // Update the persistent max
    data->max = std::min(distance, data->max);

    // Return the final Damerau-Levenshtein distance
    return static_cast<long long>(distance);
}
