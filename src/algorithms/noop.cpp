/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    31 July 2019 - Robert Jacobson

    This is a no-op UDF function that just returns as soon as it's called.
    It's purpose is to provide a lower bound for benchmarking purposes.

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

// Limits
#ifndef NOOP_BUFFER_SIZE
// 640k should be good enough for anybody.
#define NOOP_BUFFER_SIZE 512ull
#endif
constexpr long long NOOP_MAX_EDIT_DIST = std::max(0ull,
                                                       std::min(16384ull, NOOP_BUFFER_SIZE));

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
    NOOP_ARG_NUM_ERROR[] = "Wrong number of arguments. NOOP() requires three arguments:\n"
                                "\t1. A string\n"
                                "\t2. A string\n"
                                "\t3. A maximum distance (0 <= int < ${NOOP_MAX_EDIT_DIST}).";
constexpr const auto NOOP_ARG_NUM_ERROR_LEN = std::size(NOOP_ARG_NUM_ERROR) + 1;
constexpr const char NOOP_MEM_ERROR[] = "Failed to allocate memory for NOOP"
                                             " function.";
constexpr const auto NOOP_MEM_ERROR_LEN = std::size(NOOP_MEM_ERROR) + 1;
constexpr const char
    NOOP_ARG_TYPE_ERROR[] = "Arguments have wrong type. NOOP() requires three arguments:\n"
                                 "\t1. A string\n"
                                 "\t2. A string\n"
                                 "\t3. A maximum distance (0 <= int < ${NOOP_MAX_EDIT_DIST}).";
constexpr const auto NOOP_ARG_TYPE_ERROR_LEN = std::size(NOOP_ARG_TYPE_ERROR) + 1;

// Use a "C" calling convention.
extern "C" {
    [[maybe_unused]] bool noop_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    [[maybe_unused]] long long noop(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
    [[maybe_unused]] void noop_deinit(UDF_INIT *initid);
}

struct DamLevConstMinData {
    // Holds the min edit distance seen so far, which is the maximum distance that can be
    // computed before the algorithm bails early.
    long long max;
    size_t const_len;
    char * const_string;
    // A buffer we only need to allocate once.
    std::vector<size_t> *buffer;
};

[[maybe_unused]]
bool noop_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, NOOP_ARG_NUM_ERROR, NOOP_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT ||
        args->arg_type[2] != INT_RESULT) {
        strncpy(message, NOOP_ARG_TYPE_ERROR, NOOP_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to allocate persistent data.
    DamLevConstMinData *data = new DamLevConstMinData();
    // (NOOP_MAX_EDIT_DIST);
    if (nullptr == data) {
        strncpy(message, NOOP_MEM_ERROR, NOOP_MEM_ERROR_LEN);
        return 1;
    }
    // Initialize persistent data.
    initid->ptr = (char *)data;
    data->max = NOOP_MAX_EDIT_DIST;
    data->buffer = new std::vector<size_t>(NOOP_MAX_EDIT_DIST);
    data->const_string = new(std::nothrow) char[NOOP_MAX_EDIT_DIST];
    if (nullptr == data->const_string) {
        strncpy(message, NOOP_MEM_ERROR, NOOP_MEM_ERROR_LEN);
        return 1;
    }
    // Initialized on first call to noop.
    data->const_len = 0;

    // noop does not return null.
    initid->maybe_null = 0;
    return 0;
}

[[maybe_unused]]
void noop_deinit(UDF_INIT *initid) {
    DamLevConstMinData &data = *(DamLevConstMinData *)initid->ptr;
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

[[maybe_unused]]
long long noop(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, [[maybe_unused]] char *error) {
    // Retrieve the arguments, setting maximum edit distance and the strings accordingly.
    if ((long long *)args->args[2] == 0) {
        // This is the case that the user gave 0 as max distance.
        return 0ll;
    }

    // Retrieve the persistent data.
    DamLevConstMinData &data = *(DamLevConstMinData *)initid->ptr;

    // Retrieve buffer.
    //std::vector<size_t> &buffer = *data.buffer; // Initialized later.
    // Retrieve max edit distance.
    long long &max = data.max;

    // For purposes of the algorithm, set max to the smallest distance seen so far.
    max = std::min(*((long long *)args->args[2]), max);
    if (args->args[0] == nullptr || args->lengths[0] == 0 || args->args[1] == nullptr ||
        args->lengths[1] == 0) {
        // Either one of the strings doesn't exist, or one of the strings has
        // length zero. In either case
        return (long long)std::max(args->lengths[0], args->lengths[1]);
    }

    return 0;
}
