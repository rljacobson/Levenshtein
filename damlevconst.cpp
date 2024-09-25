#include "common.h"
#include <memory>
#include <iostream>


// Use a "C" calling convention for MySQL UDF functions.
extern "C" {
    bool damlevconst_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    int damlevconst(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
    void damlevconst_deinit(UDF_INIT *initid);
}

struct PersistentData {
    long long max;
    size_t const_len;
    std::unique_ptr<char[]> const_string;
    std::unique_ptr<std::vector<size_t>> buffer;
};

bool damlevconst_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        set_error(message, "DAMLEVCONST() requires 3 arguments: two strings and a max distance.");
        return 1;
    }
    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != INT_RESULT) {
        set_error(message, "Wrong argument types for DAMLEVCONST(). Expected: 2 strings, 1 max distance.");
        return 1;
    }


    // Allocate persistent data using smart pointers
    auto data = std::make_unique<PersistentData>();

    // Initialize persistent data
    data->max = 512;  // Example max value, could be dynamic
    data->buffer = std::make_unique<std::vector<size_t>>(data->max);
    data->const_string = std::make_unique<char[]>(data->max);  // Pre-allocate buffer for constant string

    // If memory allocation failed
    if (!data->const_string || !data->buffer) {
        set_error(message, "Memory allocation failure in DAMLEVCONST.");
        return 1;
    }

    data->const_len = 0;
    initid->ptr = reinterpret_cast<char*>(data.release());

    // damlevconst does not return null
    initid->maybe_null = 0;
    return 0;
}

void damlevconst_deinit(UDF_INIT *initid) {
    // Clean up by destroying the unique_ptrs automatically
    delete reinterpret_cast<PersistentData*>(initid->ptr);
}

int damlevconst(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error) {
    if (!args->args[2]) {
        // Early exit if max distance is zero
        return 0;
    }

    PersistentData& data = *reinterpret_cast<PersistentData*>(initid->ptr);
    long long& max = data.max;
    std::vector<size_t>& buffer = *data.buffer;

    // Validate max distance and update
    if (args->args[2]) {
        long long user_max = *((long long*)args->args[2]);
        if (user_max < 0) {
            set_error(error, "Maximum edit distance cannot be negative.");
            *is_null = 1;
            return 1;
        }
        max = std::min(user_max, max);
    }

    // Handle null or empty strings
    if (!args->args[0] || args->lengths[0] == 0 || !args->args[1] || args->lengths[1] == 0) {
        return (int) std::max(args->lengths[0], args->lengths[1]);
    }

    if (data.const_len == 0) {
        data.const_len = args->lengths[1];
        strncpy(data.const_string.get(), args->args[1], data.const_len);
        data.const_string[data.const_len] = '\0';
    }

    // Let's make some string views so we can use the STL.
    std::string_view query{data.const_string.get(), data.const_len};
    std::string_view subject{args->args[0], args->lengths[0]};

    // Skip any common prefix.
    auto prefix_mismatch = std::mismatch(subject.begin(), subject.end(), query.begin(), query.end());
    auto start_offset = static_cast<size_t>(std::distance(subject.begin(), prefix_mismatch.first));

    // If one of the strings is a prefix of the other, return the length difference.
    if (subject.length() == start_offset) {
        return int(query.length()) - int(start_offset);
    } else if (query.length() == start_offset) {
        return int(subject.length()) - int(start_offset);
    }

    // Skip any common suffix.
    auto suffix_mismatch = std::mismatch(subject.rbegin(), std::next(subject.rend(), -int(start_offset)),
                                         query.rbegin(), std::next(query.rend(), -int(start_offset)));
    auto end_offset = std::distance(subject.rbegin(), suffix_mismatch.first);

    // Extract the different part if significant.
    if (start_offset + end_offset < subject.length()) {
        subject = subject.substr(start_offset, subject.length() - start_offset - end_offset);
        query = query.substr(start_offset, query.length() - start_offset - end_offset);
    }

    // Ensure 'subject' is the smaller string for efficiency
    if (query.length() < subject.length()) {
        std::swap(subject, query);
    }

    int n = static_cast<int>(subject.length()); // Cast size_type to int
    int m = static_cast<int>(query.length()); // Cast size_type to int

    // Determine the effective maximum edit distance
    int effective_max = std::min(static_cast<int>(max), n);


    // Re-initialize buffer before calculation
    std::iota(buffer.begin(), buffer.begin() + static_cast<int>(query.length()) + 1, 0);

    // Resize the buffer to simulate a 2D matrix with dimensions (n+1) x (m+1)
    buffer.resize((n + 1) * (m + 1));

    // Lambda function for 2D matrix indexing in the 1D buffer
    auto idx = [m](int i, int j) { return i * (m + 1) + j; };

    // Initialize the first row and column of the matrix
    for (int i = 0; i <= n; ++i) {
        buffer[idx(i, 0)] = i;
    }
    for (int j = 0; j <= m; ++j) {
        buffer[idx(0, j)] = j;
    }

    // Main loop to calculate the Damerau-Levenshtein distance
    for (int i = 1; i <= n; ++i) {
        size_t column_min = std::numeric_limits<size_t>::max();

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
        if (column_min > static_cast<size_t>(effective_max)) {
            return max + 1;
        }
    }

    // Return the final Damerau-Levenshtein distance
    return static_cast<int>(buffer[idx(n, m)]);
}
