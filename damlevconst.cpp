#include "common.h"
#include <memory>
#include <iostream>

// Use a "C" calling convention for MySQL UDF functions.
extern "C" {
    int damlevconst_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long damlevconst(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
    void damlevconst_deinit(UDF_INIT *initid);
}

int damlevconst_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strcpy(message, "DAMLEVCONST() requires 3 arguments: two strings and a max distance.");
        return 1;
    }
    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != INT_RESULT) {
        strcpy(message, "Wrong argument types for DAMLEVCONST(). Expected: 2 strings, 1 max distance.");
        return 1;
    }

    // Initialize persistent data
    std::vector<int>* buffer = new (std::nothrow) std::vector<int>(DAMLEV_MAX_EDIT_DIST);

    // If memory allocation failed
    if (!buffer) {
        strcpy(message, "Memory allocation failure in DAMLEVCONST.");
        return 1;
    }

    initid->ptr = reinterpret_cast<char*>(buffer);

    // damlevconst does not return null
    initid->maybe_null = 0;
    return 0;
}

void damlevconst_deinit(UDF_INIT *initid) {
    delete reinterpret_cast<std::vector<int>*>(initid->ptr);
}

long long damlevconst(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error) {
    if (!args->args[2]) {
        // Early exit if max distance is zero
        return 0;
    }

    // Fetch persistent buffer allocation
    std::vector<int>& buffer = *reinterpret_cast<std::vector<int>*>(initid->ptr);

    // Handle null or empty strings
    if (!args->args[0] || args->lengths[0] == 0 || !args->args[1] || args->lengths[1] == 0) {
        return (int) std::max(args->lengths[0], args->lengths[1]);
    }

    // Validate max distance and update
    long long max = DAMLEV_MAX_EDIT_DIST;
    if (args->args[2]) {
        long long user_max = *((long long*)args->args[2]);
        if (user_max < 0) {
            strcpy(error, "Maximum edit distance cannot be negative.");
            return 1;
        }
        max = std::min(user_max, max);
    }


    // Let's make some string views so we can use the STL.
    std::string_view query{  args->args[1], args->lengths[1]};
    std::string_view subject{args->args[0], args->lengths[0]};

    // Scope for common prefix/suffix trimming
    {
        // Find the start offset of the mismatch
        auto prefix_mismatch = std::mismatch(subject.begin(), subject.end(), query.begin(), query.end());
        auto start_offset = std::distance(subject.begin(), prefix_mismatch.first);

        // Get the lengths of both strings
        auto subject_length = subject.length();
        auto query_length = query.length();

        // If one of the strings is a prefix of the other, return the length difference
        if (start_offset == static_cast<long>(subject_length)) {
            return static_cast<long long>(query_length) - start_offset;
        } else if (start_offset == static_cast<long>(query_length)) {
            return static_cast<long long>(subject_length) - start_offset;
        }

        // Find the end offset of the mismatch (reverse comparison)
        auto suffix_mismatch = std::mismatch(subject.rbegin(), subject.rbegin() + (static_cast<long>(subject_length) - start_offset),
                                                      query.rbegin(),   query.rbegin()   + (static_cast<long>(query_length)   - start_offset));
        auto end_offset = std::distance(subject.rbegin(), suffix_mismatch.first);

        // Extract the different part if significant
        if (start_offset + end_offset < subject_length) {
            subject = subject.substr(start_offset, subject_length - start_offset - end_offset);
            query   = query.substr(  start_offset, query_length   - start_offset - end_offset);
        }
    }

    // Ensure 'subject' is the smaller string for efficiency
    if (query.length() < subject.length()) {
        std::swap(subject, query);
    }

    int n = static_cast<int>(subject.length());
    int m = static_cast<int>(query.length());

    // Determine the effective maximum edit distance
    int effective_max = std::min(static_cast<int>(max), n);

    // Re-initialize buffer before calculation
    std::iota(buffer.begin(), buffer.begin() + static_cast<int>(query.length()) + 1, 0);

    // Lambda function for 2D matrix indexing in the 1D buffer
    auto idx = [m](int i, int j) { return i * (m + 1) + j; };

    // Main loop to calculate the Damerau-Levenshtein distance
    for (int i = 1; i <= n; ++i) {
        // Set column_min to infinity
        int column_min = std::numeric_limits<int>::max();
        // Initialize first column
        buffer[idx(i, 0)] = i;

        for (int j = 1; j <= m; ++j) {
            int cost = (subject[i - 1] == query[j - 1]) ? 0 : 1;

            buffer[idx(i, j)] = std::min({buffer[idx(i - 1, j)] + 1,
                                          buffer[idx(i, j - 1)] + 1,
                                          buffer[idx(i - 1, j - 1)] + cost});

            // Check for transpositions
            if (i > 1 && j > 1 && subject[i - 1] == query[j - 2] && subject[i - 2] == query[j - 1]) {
                buffer[idx(i, j)] = std::min(buffer[idx(i, j)], buffer[idx(i - 2, j - 2)] + cost);
            }

            column_min = std::min(column_min, buffer[idx(i, j)]);
        }

        // Early exit if the minimum edit distance exceeds the effective maximum
        if (column_min > static_cast<int>(effective_max)) {
            return max + 1;
        }
    }

    // Return the final Damerau-Levenshtein distance
    return static_cast<int>(buffer[idx(n, m)]);
}
