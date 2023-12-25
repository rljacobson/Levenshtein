/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    17 January 2019

    This implementation is better than most others out there. It is extremely
    fast and efficient.
            __—R.__

    <hr>
    `DAMLEVCONST()` computes the Damarau Levenshtein edit distance between two strings when the
    edit distance is less than a given number.

    Syntax:

        DAMLEVCONST(String1, ConstString, PosInt);

    `String1`:  A string constant or column.
    `ConstString`:  A string constant to be compared to `String1`.
    `PosInt`:   A positive integer. If the distance between `String1` and
                `ConstString` is greater than `PosInt`, `DAMLEVCONST()` will stop its
                computation at `PosInt` and return `PosInt`. Make `PosInt` as
                small as you can to improve speed and efficiency. For example,
                if you put `WHERE DAMLEVCONST(...) < k` in a `WHERE`-clause, make
                `PosInt` be `k`.

    Returns: Either an integer equal to the edit distance between `String1` and `String2` or `PosInt`,
    whichever is smaller.

    Example Usage:

        SELECT Name, DAMLEVCONST(Name, "Vladimir Iosifovich Levenshtein", 6) AS
            EditDist FROM CUSTOMERS WHERE DAMLEVCONST(Name, "Vladimir Iosifovich Levenshtein", 6) <= 6;

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

#include <iostream>
#include "common.h"
//#define PRINT_DEBUG
#ifdef PRINT_DEBUG
#include <iostream>
#endif

// Limits
#ifndef DAMLEVCONST_BUFFER_SIZE
    // 640k should be good enough for anybody.
    #define DAMLEVCONST_BUFFER_SIZE 512ull
#endif
constexpr long long DAMLEVCONST_MAX_EDIT_DIST = std::max(0ull,
        std::min(16384ull, DAMLEVCONST_BUFFER_SIZE));

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char DAMLEVCONST_ARG_NUM_ERROR[] =
        "DAMLEVCONST() requires 3 arguments: two strings and a max distance.";
constexpr const auto DAMLEVCONST_ARG_NUM_ERROR_LEN = std::size(DAMLEVCONST_ARG_NUM_ERROR) + 1;

constexpr const char DAMLEVCONST_MEM_ERROR[] =
        "Memory allocation failure in DAMLEVCONST.";
constexpr const auto DAMLEVCONST_MEM_ERROR_LEN = std::size(DAMLEVCONST_MEM_ERROR) + 1;

constexpr const char DAMLEVCONST_ARG_TYPE_ERROR[] =
        "Wrong argument types for DAMLEVCONST(). Expected: 2 strings, 1 max distance.";
constexpr const auto DAMLEVCONST_ARG_TYPE_ERROR_LEN = std::size(DAMLEVCONST_ARG_TYPE_ERROR) + 1;

// Use a "C" calling convention.
extern "C" {
bool damlevconst_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
int damlevconst(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevconst_deinit(UDF_INIT *initid);
}

struct PersistentData {
    // Holds the min edit distance seen so far, which is the maximum distance that can be
    // computed before the algorithm bails early.
    long long max;
    size_t const_len;
    char * const_string;
    // A buffer we only need to allocate once.
    std::vector<size_t> *buffer;
};

bool damlevconst_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, DAMLEVCONST_ARG_NUM_ERROR, DAMLEVCONST_ARG_NUM_ERROR_LEN);
        return int (1);
    }
    // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT ||
            args->arg_type[2] != INT_RESULT) {
        strncpy(message, DAMLEVCONST_ARG_TYPE_ERROR, DAMLEVCONST_ARG_TYPE_ERROR_LEN);
        return int (1);
    }

    // Attempt to allocate persistent data.
    auto *data = new PersistentData();

    // Initialize persistent data.
    initid->ptr = (char *)data;
    data->max = DAMLEVCONST_MAX_EDIT_DIST;
    data->buffer = new std::vector<size_t>(data->max);
    data->const_string = new(std::nothrow) char[DAMLEVCONST_MAX_EDIT_DIST];
    if (nullptr == data->const_string) {
        strncpy(message, DAMLEVCONST_MEM_ERROR, DAMLEVCONST_MEM_ERROR_LEN);
        return int(1);
    }
    // Initialized on first call to damlevconst.
    data->const_len = 0;

    // damlevconst does not return null.
    initid->maybe_null = 0;
    return int(0);
}

void damlevconst_deinit(UDF_INIT *initid) {
    PersistentData &data = *(PersistentData *)initid->ptr;
    if(nullptr != data.const_string){
        delete[] data.const_string;
        data.const_string = nullptr;
    }
    if(nullptr != data.buffer){
        delete data.buffer;
        data.buffer = nullptr;
    }
    delete[] initid->ptr;
}
int damlevconst(UDF_INIT *initid, UDF_ARGS *args, UNUSED char *is_null, UNUSED char *error) {


    // Retrieve the arguments, setting maximum edit distance and the strings accordingly.
    if ((long long *) args->args[2] == 0) {
        // This is the case that the user gave 0 as max distance.

        return 0ll;
    }


    // Retrieve the persistent data.
    PersistentData &data = *(PersistentData *) initid->ptr;
    long long &max = data.max;
    // Retrieve buffer.
    std::vector<size_t> &buffer = *data.buffer; // Initialized later.



    // For purposes of the algorithm, set max to the smallest distance seen so far.
    // Validate and update max with user input
    if (args->args[2] != nullptr) {
        long long user_max = *((long long *) args->args[2]);
        if (user_max < 0) {
            // If user-supplied max is negative, set an appropriate error
            strncpy(error, "Maximum edit distance cannot be negative.", MYSQL_ERRMSG_SIZE);
            *is_null = 1; // Indicate a NULL return value
            return int(1); // Return 1 to indicate an error occurred
        }
        // Set max to the smaller of the user_max and the current max
        max = std::min(user_max, max);
    }

    if (args->args[0] == nullptr || args->lengths[0] == 0 || args->args[1] == nullptr ||
        args->lengths[1] == 0) {
        // Either one of the strings doesn't exist, or one of the strings has
        // length zero. In either case

        return (int) std::max(args->lengths[0], args->lengths[1]);
    }
    int max_string_length = int(std::max(args->lengths[0], args->lengths[1]));


#ifdef PRINT_DEBUG
    std::cout << "subject= " <<args->args[0] <<std::endl;
    std::cout <<"constant query= " <<args->args[1]<<std::endl;
    std::cout << "Maximum edit distance:" <<  std::min(*((long long *)args->args[2]),
                                                    DAMLEVCONST_MAX_EDIT_DIST)<<std::endl;
    std::cout << "DAMLEVCONST_MAX_EDIT_DIST:" <<DAMLEVCONST_MAX_EDIT_DIST<<std::endl;
    std::cout << "Max String Length:" << static_cast<double>(std::max(args->lengths[0],
                                                                     args->lengths[1]))<<std::endl;

#endif

    // Check if initialization of persistent data is required. We cannot do this in
    // damlevconst_init, because we do not know what the constant is yet in damlevconst_init.
    if (0 == data.const_len) {
        // Only done once.
        data.const_len = args->lengths[1];
        strncpy(data.const_string, args->args[1], data.const_len);
        // Null terminate the string.
        data.const_string[data.const_len] = '\0';

    }
    // Let's make some string views so we can use the STL.
    /// *****  FOR UNITTEST you must disable the const_string on the query
    std::string_view query{data.const_string, data.const_len};
    /// uncomment the below for UNITTEST.
    //std::string_view query{args->args[1], args->lengths[1]};

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

    int n = static_cast<int>(subject.size()); // Cast size_type to int
    int m = static_cast<int>(query.size()); // Cast size_type to int

// Calculate trimmed_max based on the lengths of the trimmed strings
    auto trimmed_max = std::max(n, m);
   // std::cout << "max" <<max<<std::endl;
    //std::cout << "trimmed max length:" <<trimmed_max<<std::endl;
    //std::cout << "trimmed subject= " <<subject <<std::endl;
    //std::cout <<"trimmed constant query= " <<query<<std::endl;

// Determine the effective maximum edit distance
// Casting max to int (ensure that max is within the range of int)
    int effective_max = std::min(static_cast<int>(max), static_cast<int>(trimmed_max));

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

            return max_string_length;
        }
    }
    data.buffer->resize(DAMLEVCONST_MAX_EDIT_DIST);
    return (static_cast<int>(buffer[idx(n, m)]));
}
