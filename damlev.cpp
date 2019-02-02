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
            EditDist FROM CUSTOMERS WHERE EditDist < 6;

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

#include <algorithm>
#include <vector>
#include <string>
#include <string_view>
#include <cstring>
#include <iterator>
#include <numeric>

#ifdef PRINT_DEBUG
#include <iostream>
#endif

#include "mysql.h"

// Limits
#ifndef DAMLEV_BUFFER_SIZE
    // 640k should be good enough for anybody.
    #define DAMLEV_BUFFER_SIZE 512ull
#endif
constexpr long long DAMLEV_MAX_EDIT_DIST = std::max(0ull, std::min(16384ull,  DAMLEV_BUFFER_SIZE));

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
    // We require 3 arguments:
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

long long damlev(UDF_INIT *initid, UDF_ARGS *args, __attribute__((unused)) char *is_null,
        __attribute__((unused)) char *error) {
    // Retrieve the arguments.
    if (args->lengths[0] == 0 || args->lengths[1] == 0 || args->args[1] == nullptr
            || args->args[0] == nullptr) {
        // Either one of the strings doesn't exist, or one of the strings has
        // length zero. In either case
        return (long long)std::max(args->lengths[0], args->lengths[1]);
    }
    // Retrieve buffer.
    std::vector<size_t> &buffer = *(std::vector<size_t> *)initid->ptr;

    // Let's make some string views so we can use the STL.
    std::string_view subject{args->args[0], args->lengths[0]};
    std::string_view query{args->args[1], args->lengths[1]};

    // Skip any common prefix.
    auto[subject_begin, query_begin] =
    std::mismatch(subject.begin(), subject.end(), query.begin(), query.end());
    auto start_offset = (size_t)std::distance(subject.begin(), subject_begin);

    // If one of the strings is a prefix of the other, done.
    if (subject.length() == start_offset) {
        return (long long)(query.length() - start_offset);
    } else if (query.length() == start_offset) {
        return (long long)(subject.length() - start_offset);
    }

    // Skip any common suffix.
    auto[subject_end, query_end] = std::mismatch(
            subject.rbegin(), static_cast<decltype(subject.rend())>(subject_begin - 1),
            query.rbegin(), static_cast<decltype(query.rend())>(query_begin - 1));
    auto end_offset = std::min((size_t)std::distance(subject.rbegin(), subject_end),
                               (size_t)(subject.size() - start_offset));

    // Take the different part.
    subject = subject.substr(start_offset, subject.size() - end_offset - start_offset + 1);
    query = query.substr(start_offset, query.size() - end_offset - start_offset + 1);

    // Make "subject" the smaller one.
    if (query.length() < subject.length()) {
        std::swap(subject, query);
    }
    // If one of the strings is a suffix of the other.
    if (subject.length() == 0) {
        return static_cast<long long>(query.length());
    }

    // Init buffer.
    std::iota(buffer.begin(), buffer.begin() + query.length() + 1, 0);
    size_t end_j; // end_j is referenced after the loop.
    for (size_t i = 1; i < subject.length() + 1; ++i) {
        // temp = i - 1;
        size_t temp = buffer[0]++;
        size_t prior_temp = 0;
        #ifdef PRINT_DEBUG
        std::cout << i << ":  " << temp << "  ";
        #endif

        // Setup for max distance, which only needs to look in the window
        // between i-max <= j <= i+max.
        // The result of the max is positive, but we need the second argument
        // to be allowed to be negative.
        const size_t start_j = static_cast<size_t>(std::max(1ll,
                                               (static_cast<long long>(i) - DAMLEV_MAX_EDIT_DIST/2)));
        end_j = std::min(static_cast<size_t>(query.length() + 1),
                static_cast<size_t >(i + DAMLEV_MAX_EDIT_DIST/2));
        size_t column_min = static_cast<size_t>(DAMLEV_MAX_EDIT_DIST);
        for (size_t j = start_j; j < end_j; ++j) {
            const size_t p = temp; // p = buffer[j - 1];
            const size_t r = buffer[j];
            temp = std::min(std::min(r,  // Insertion.
                                     p   // Deletion.
                            ) + 1,

                            std::min(
                                    // Transposition.
                                    prior_temp + 1,
                                    // Substitution.
                                    temp + (subject[i - 1] == query[j - 1] ? 0 : 1)
                            )
            );
            // Keep track of column minimum.
            if (temp < column_min) {
                column_min = temp;
            }
            // Record matrix value mat[i-2][j-2].
            prior_temp = temp;
            std::swap(buffer[j], temp);
            #ifdef PRINT_DEBUG
            std::cout << temp << "  ";
            #endif
        }
        if (column_min >= DAMLEV_MAX_EDIT_DIST) {
            // There is no way to get an edit distance > column_min.
            // We can bail out early.
            return DAMLEV_MAX_EDIT_DIST;
        }
        #ifdef PRINT_DEBUG
        std::cout << std::endl;
        #endif
    }

    return buffer[end_j-1];
}
