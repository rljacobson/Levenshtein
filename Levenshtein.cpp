/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    17 January 2019

    This implementation is better than most others out there. It is extremely
    fast and efficient.
            __—R.__

    <hr>
    Syntax:

        DAMLEV(String1, String2, PosInt);

    `String1`:  A string constant or column.
    `String2`:  A string constant or column which will be compared to `String1`.
    `PosInt`:   A positive integer. If the distance between `String1` and
                `String2` is greater than `PosInt`, `DAMLEV()` will stop its
                computation at `PosInt` and return `PosInt`. Make `PosInt` as
                small as you can to improve speed and efficiency. For example,
                if you put `WHERE DAMLEV(...) < k` in a `WHERE`-clause, make
                `PosInt` be `k`.

    Example Usage:

        SELECT Name, DAMLEV(Name, "Vladimir Iosifovich Levenshtein", 6) AS
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
#include <iterator>
#include <numeric>

#include "mysql.h"

// Limits
#ifndef DAMLEV_BUFFER_SIZE
    // 640k should be good enough for anybody.
    #define DAMLEV_BUFFER_SIZE 500ll
#endif
constexpr size_t MAX_EDIT_DIST = std::max(0ll, std::min(1000000000ll, DAMLEV_BUFFER_SIZE));

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        DAMLEV_ARG_NUM_ERROR[] = "Wrong number of arguments. DAMLEV() requires three arguments:\n"
                                 "\t1. String from field.\n"
                                 "\t2. Constant string.\n"
                                 "\t3. Maximum distance (0 <= int < 256).";
constexpr const auto DAMLEV_ARG_NUM_ERROR_LEN = std::size(DAMLEV_ARG_NUM_ERROR) + 1;
constexpr const char DAMLEV_MEM_ERROR[] = "Failed to allocate memory for DAMLEV"
                                          " function.";
constexpr const auto DAMLEV_MEM_ERROR_LEN = std::size(DAMLEV_MEM_ERROR) + 1;
constexpr const char
        DAMLEV_ARG_TYPE_ERROR[] = "Arguments have wrong type. DAMLEV() requires three arguments:\n"
                                  "\t1. String from field.\n"
                                  "\t2. Constant string.\n"
                                  "\t3. Maximum distance (0 <= int < 256).";
constexpr const auto DAMLEV_ARG_TYPE_ERROR_LEN = std::size(DAMLEV_ARG_TYPE_ERROR) + 1;

// Use a "C" calling convention.
extern "C" {
bool damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev_deinit(UDF_INIT *initid);
}

bool damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, DAMLEV_ARG_NUM_ERROR, DAMLEV_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT ||
            args->arg_type[2] != INT_RESULT) {
        strncpy(message, DAMLEV_ARG_TYPE_ERROR, DAMLEV_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to allocate a buffer.
    initid->ptr = (char *)new(std::nothrow) std::vector<size_t>(MAX_EDIT_DIST);
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

long long damlev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error) {
    // Retrieve the arguments.
    // Maximum edit distance.
    long long max;
    max = std::min(*((long long *)args->args[2]),
                   DAMLEV_BUFFER_SIZE);
    if (max == 0) {
        return 0ll;
    }
    if (args->args[0] == nullptr || args->lengths[0] == 0 || args->args[1] == nullptr ||
            args->lengths[1] == 0) {
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
        return query.length();
    }

    // Init buffer.
    std::iota(buffer.begin(), buffer.begin() + query.length() + 1, 0);

    size_t temp, prior_temp = 0, end_j, start_j, column_min, p=0, r;
    for (size_t i = 1; i < subject.length() + 1; ++i) {
        // temp = i - 1;
        temp = buffer[0]++;
        #ifdef PRINT_DEBUG
        std::cout << i << ":  " << temp << "  ";
        #endif

        // Setup for max distance, which only needs to look in the window
        // between i-max <= j <= i+max.
        // The result of the max is positive, but we need the second argument
        // to be allowed to be negative.
        start_j = static_cast<size_t>(std::max(1ll, (static_cast<long long>(i) -
                static_cast<long long>(MAX_EDIT_DIST))));
        end_j = std::min(query.length() + 1, i + MAX_EDIT_DIST);
        column_min = MAX_EDIT_DIST;

        for (size_t j = start_j; j < end_j; ++j) {
            p = temp; // p = buffer[j - 1];
            r = buffer[j];
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
        if (column_min >= MAX_EDIT_DIST) {
            // There is no way to get an edit distance > column_min.
            // We can bail out early.
            return MAX_EDIT_DIST;
        }
        #ifdef PRINT_DEBUG
        std::cout << std::endl;
        #endif
    }

    return buffer[end_j-1];
}
