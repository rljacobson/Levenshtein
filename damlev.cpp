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
#include <iostream>
#include <cmath>

#include "common.h"
//#define PRINT_DEBUG
#ifdef PRINT_DEBUG
#include <iostream>
#endif

//#define PRINT_DEBUG
// Limits
#ifndef DAMLEV_BUFFER_SIZE
    // 640k should be good enough for anybody.
    #define DAMLEV_BUFFER_SIZE 512ull
#endif
constexpr long long DAMLEV_MAX_EDIT_DIST = std::max(0ull, std::min(16384ull, DAMLEV_BUFFER_SIZE));

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
bool damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev_deinit(UDF_INIT *initid);
}

bool damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 2 arguments:
    if (args->arg_count != 2) {
        strncpy(message, DAMLEV_ARG_NUM_ERROR, DAMLEV_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT) {
        strncpy(message, DAMLEV_ARG_TYPE_ERROR, DAMLEV_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to allocate a buffer.
    initid->ptr = (char *)new(std::nothrow) std::vector<size_t>((DAMLEV_MAX_EDIT_DIST));
    if (initid->ptr == nullptr) {
        strncpy(message, DAMLEV_MEM_ERROR, DAMLEV_MEM_ERROR_LEN);
        return 1;
    }

    // damlev does not return null.
    initid->maybe_null = 0;
    return 0;
}

void damlev_deinit(UDF_INIT *initid) {
    delete[] initid->ptr;
}

long long damlev(UDF_INIT *initid, UDF_ARGS *args, UNUSED char *is_null, UNUSED char *error) {
    // Retrieve the arguments.

    //set max string lenght for later
    const double max_string_length = static_cast<double>(std::max(args->lengths[0],
                                                                  args->lengths[1]));
    // we don't get a LD limit, so set at max string lenght
    //const long long int max = max_string_length;

    if (args->lengths[0] == 0 || args->lengths[1] == 0 || args->args[1] == nullptr
        || args->args[0] == nullptr) {
        // Either one of the strings doesn't exist, or one of the strings has
        // length zero. In either case
        return (long long) std::max(args->lengths[0], args->lengths[1]);
    }
#ifdef PRINT_DEBUG
    std::cout << "Maximum edit distance:" <<  std::min(*((long long *)args->args[2]),
                                                       DAMLEV_MAX_EDIT_DIST)<<std::endl;
    std::cout << "DAMLEVCONST_MAX_EDIT_DIST:" <<DAMLEV_MAX_EDIT_DIST<<std::endl;
    std::cout << "Max String Length:" << static_cast<double>(std::max(args->lengths[0],
                                                                      args->lengths[1]))<<std::endl;

#endif
    // Retrieve buffer.
    std::vector<size_t> &buffer = *(std::vector<size_t> *) initid->ptr;

    // Let's make some string views so we can use the STL.
    std::string_view subject{args->args[0], args->lengths[0]};
    std::string_view query{args->args[1], args->lengths[1]};

    // Skip any common prefix.
    auto [subject_begin, query_begin] =
            std::mismatch(subject.begin(), subject.end(), query.begin(), query.end());
    auto start_offset = (size_t) std::distance(subject.begin(), subject_begin);

    // If one of the strings is a prefix of the other, done.
    if (subject.length() == start_offset) {
#ifdef PRINT_DEBUG
        std::cout << subject << " is a prefix of " << query << ", bailing" << std::endl;
#endif
        return (size_t) (query.length() - start_offset);
    } else if (query.length() == start_offset) {
#ifdef PRINT_DEBUG
        std::cout << query << " is a prefix of " << subject << ", bailing" << std::endl;
#endif
        return (size_t) (subject.length() - start_offset);
    }

    // Skip any common suffix.
    auto [subject_end, query_end] = std::mismatch(
            subject.rbegin(), static_cast<decltype(subject.rend())>(subject_begin),
            query.rbegin(), static_cast<decltype(query.rend())>(query_begin));
    auto end_offset = std::min((size_t) std::distance(subject.rbegin(), subject_end),
                               (size_t) (subject.size() - start_offset));

#ifdef PRINT_DEBUG
    // Printing before trimming
    std::cout << "Before Trimming:\n";
    std::cout << "subject (original): " << subject << std::endl;
    std::cout << "query (original): " << query << std::endl;
    std::cout << "start_offset: " << start_offset << std::endl;
    std::cout << "end_offset: " << end_offset << std::endl;
    std::cout << "subject.size(): " << subject.size() << std::endl;
    std::cout << "query.size(): " << query.size() << std::endl;
#endif

    // Take the different part.
    //subject = subject.substr(start_offset, subject.size() - end_offset - start_offset);
    //query = query.substr(start_offset, query.size() - end_offset - start_offset);

    if (start_offset > 2 && end_offset > 2) {
        subject = subject.substr(start_offset, subject.size() - end_offset - start_offset);
        query = query.substr(start_offset, query.size() - end_offset - start_offset);
    }


    int trimmed_max = std::max(int(query.length()), int(subject.length()));
    int max = trimmed_max;
#ifdef PRINT_DEBUG
    std::cout << "trimmed max length:" <<trimmed_max<<std::endl;
    std::cout << "trimmed subject= " <<subject <<std::endl;
    std::cout <<"trimmed constant query= " <<query<<std::endl;
#endif

    // Make "subject" the smaller one.
    if (query.length() < subject.length()) {

        std::swap(subject, query);

    }


    // If one of the strings is a suffix of the other.
    if (subject.length() == 0) {
#ifdef PRINT_DEBUG
        std::cout << subject << " is a suffix of " << query << ", bailing" << std::endl;
#endif
        return query.length();
    }

    int n = subject.size();
    int m = query.size();

    // Resize buffer to simulate a 2D matrix
    buffer.resize((n + 1) * (m + 1));

    // Function to access the 2D matrix simulated in a 1D vector
    auto idx = [m](int i, int j) { return i * (m + 1) + j; };

    // Initialize first row and column
    for (int i = 0; i <= n; i++) {
        buffer[idx(i, 0)] = i;
    }
    for (int j = 0; j <= m; j++) {
        buffer[idx(0, j)] = j;
    }

    for (int i = 1; i <= n; i++) {
        size_t column_min = trimmed_max + 1;
        for (int j = 1; j <= m; j++) {
            int cost = (subject[i - 1] == query[j - 1]) ? 0 : 1;

            buffer[idx(i, j)] = std::min({ buffer[idx(i - 1, j)] + 1,
                                           buffer[idx(i, j - 1)] + 1,
                                           buffer[idx(i - 1, j - 1)] + cost });

            if (i > 1 && j > 1 && subject[i - 1] == query[j - 2] && subject[i - 2] == query[j - 1]) {
                buffer[idx(i, j)] = std::min(buffer[idx(i, j)], buffer[idx(i - 2, j - 2)] + cost);
            }

            column_min = std::min(column_min, buffer[idx(i, j)]);
        }

        if (column_min > max) {
            return max_string_length;
        }
    }
    buffer.resize(DAMLEV_MAX_EDIT_DIST);
    return buffer[idx(n, m)];
}