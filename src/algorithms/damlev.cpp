//damlev.cpp
/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    17 January 2019

    This implementation is better than most others out there. It is extremely
    fast and efficient.
            __—R.__

    <hr>
    `DAMLEV()` computes the Damarau Levenshtein edit distance between two strings.

    Syntax:

        DAMLEV(String1, String2, PosInt);

    `String1`:  A string constant or column.
    `String2`:  A string constant or column to be compared to `String1`.

    Returns: An integer equal to the edit distance between `String1` and `String2`.

    Example Usage:

        SELECT Name, DAMLEV(Name, "Vladimir Iosifovich Levenshtein") AS
            EditDist FROM CUSTOMERS WHERE DAMLEV(Name, "Vladimir Iosifovich Levenshtein") <= 6;

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
                                 "\t1. A string.\n"
                                 "\t2. Another string.";
constexpr const auto DAMLEV_ARG_NUM_ERROR_LEN = std::size(DAMLEV_ARG_NUM_ERROR) + 1;
constexpr const char DAMLEV_MEM_ERROR[] = "Failed to allocate memory for DAMLEV"
                                          " function.";
constexpr const auto DAMLEV_MEM_ERROR_LEN = std::size(DAMLEV_MEM_ERROR) + 1;
constexpr const char
        DAMLEV_ARG_TYPE_ERROR[] = "Arguments have wrong type. DAMLEV() requires two arguments:\n"
                                  "\t1. A string.\n"
                                  "\t2. Another string.";
constexpr const auto DAMLEV_ARG_TYPE_ERROR_LEN = std::size(DAMLEV_ARG_TYPE_ERROR) + 1;

// Use a "C" calling convention.
extern "C" {
    int damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long damlev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
    void damlev_deinit(UDF_INIT *initid);
}

int damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 2 arguments:
    if (args->arg_count != 2) {
        strncpy(message, DAMLEV_ARG_NUM_ERROR, DAMLEV_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT) {
        strncpy(message, DAMLEV_ARG_TYPE_ERROR, DAMLEV_ARG_TYPE_ERROR_LEN);
        return  1;
    }

    // Attempt to allocate a buffer.
    initid->ptr = (char *)new(std::nothrow) std::vector<size_t>((DAMLEV_MAX_EDIT_DIST));
    if (initid->ptr == nullptr) {
        strncpy(message, DAMLEV_MEM_ERROR, DAMLEV_MEM_ERROR_LEN);
        return  1;
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

void damlev_deinit(UDF_INIT *initid) {
    delete[] initid->ptr;
}

long long damlev(UDF_INIT *initid, UDF_ARGS *args, UNUSED char *is_null, UNUSED char *error) {
    // Retrieve the arguments.

    //set max string lenght for later
    int max_string_length = static_cast<int>(std::max(args->lengths[0], args->lengths[1]));

    if (args->lengths[0] == 0 || args->lengths[1] == 0 || args->args[1] == nullptr
        || args->args[0] == nullptr) {
        // Either one of the strings doesn't exist, or one of the strings has
        // length zero. In either case
        return static_cast<int>(std::max(args->lengths[0], args->lengths[1]));
    }

    // Retrieve buffer.
    std::vector<size_t> &buffer = *(std::vector<size_t> *) initid->ptr;

    // Let's make some string views so we can use the STL.
    std::string_view subject{args->args[0], args->lengths[0]};
    std::string_view query{args->args[1], args->lengths[1]};


    // Skip any common prefix.
    auto prefix_mismatch = std::mismatch(subject.begin(), subject.end(), query.begin(), query.end());
    auto start_offset = std::distance(subject.begin(), prefix_mismatch.first);


// If one of the strings is a prefix of the other, return the length difference.
    if (static_cast<int>(subject.length()) == start_offset) {
        return static_cast<int>(query.length()) - start_offset;
    } else if (static_cast<int>(query.length()) == start_offset) {
        return static_cast<int>(subject.length()) - start_offset;
    }

// Skip any common suffix.
    auto suffix_mismatch = std::mismatch(subject.rbegin(), std::next(subject.rend(), -start_offset),
                                         query.rbegin(), std::next(query.rend(), -start_offset));
    auto end_offset = std::distance(subject.rbegin(), suffix_mismatch.first);

// Extract the different part if significant.
    if (start_offset + end_offset < static_cast<int>(subject.length())) {
        subject = subject.substr(start_offset, subject.length() - start_offset - end_offset);
        query = query.substr(start_offset, query.length() - start_offset - end_offset);
    }


// Ensure 'subject' is the smaller string for efficiency
    if (query.length() < subject.length()) {
        std::swap(subject, query);
    }

    int n = static_cast<int>(subject.size()); // Length of the smaller string,Cast size_type to int
    int m = static_cast<int>(query.size()); // Length of the larger string, Cast size_type to int


// Calculate trimmed_max based on the lengths of the trimmed strings
    auto trimmed_max = std::max(n, m);
    auto max = trimmed_max;
    // std::cout << "max" <<max<<std::endl;
    //std::cout << "trimmed max length:" <<trimmed_max<<std::endl;
    //std::cout << "trimmed subject= " <<subject <<std::endl;
    //std::cout <<"trimmed constant query= " <<query<<std::endl;

// Determine the effective maximum edit distance
// Casting max to int (ensure that max is within the range of int)
    int effective_max = std::min(static_cast<int>(max), static_cast<int>(trimmed_max));


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
            return max_string_length;
        }
    }
    buffer.resize(DAMLEV_MAX_EDIT_DIST);
    return (static_cast<int>(buffer[idx(n, m)]));
}
