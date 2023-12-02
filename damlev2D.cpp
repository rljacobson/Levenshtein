/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    17 January 2019

    This implementation is better than most others out there. It is extremely
    fast and efficient.
            __—R.__

    <hr>
    `DAMLEV2D()` computes the Levenshtein edit distance between two strings.

    Syntax:

        DAMLEV2D(String1, String2);

    `String1`:  A string constant or column.
    `String2`:  A string constant or column to be compared to `String1`.

    Returns: An integer equal to the edit distance between `String1` and `String2`.

    Example Usage:

        SELECT Name, DAMLEV2D(Name, "Vladimir Iosifovich Levenshtein") AS
            EditDist FROM CUSTOMERS WHERE DAMLEV2D(Name, "Vladimir Iosifovich Levenshtein") < 6;

    The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
    where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".

    <hr>

    Copyright (C) 2019 Robert Jacobson. Released under the MIT license.

    The 2 row approach used to compute edit-distance can be found here:
    https://takeuforward.org/data-structure/edit-distance-dp-33/

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
#include <vector>
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
constexpr long long EDIT_DISTANCE_MAX_EDIT_DIST = std::max(0ull, std::min(16384ull, DAMLEV_BUFFER_SIZE));

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        EDIT_DISTANCE_ARG_NUM_ERROR[] = "Wrong number of arguments. EDIT_DISTANCE() requires two arguments:\n"
                                 "\t1. A string.\n"
                                 "\t2. Another string.";
constexpr const auto EDIT_DISTANCE_ARG_NUM_ERROR_LEN = std::size(EDIT_DISTANCE_ARG_NUM_ERROR) + 1;
constexpr const char EDIT_DISTANCE_MEM_ERROR[] = "Failed to allocate memory for EDIT_DISTANCE"
                                          " function.";
constexpr const auto EDIT_DISTANCE_MEM_ERROR_LEN = std::size(EDIT_DISTANCE_MEM_ERROR) + 1;
constexpr const char
        EDIT_DISTANCE_ARG_TYPE_ERROR[] = "Arguments have wrong type. EDIT_DISTANCE() requires two arguments:\n"
                                  "\t1. A string.\n"
                                  "\t2. Another string.";
constexpr const auto EDIT_DISTANCE_ARG_TYPE_ERROR_LEN = std::size(EDIT_DISTANCE_ARG_TYPE_ERROR) + 1;

// Use a "C" calling convention.
extern "C" {
bool damlev2D_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev2D(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev2D_deinit(UDF_INIT *initid);
}

bool damlev2D_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 2 arguments:
    if (args->arg_count != 2) {
        strncpy(message, EDIT_DISTANCE_ARG_NUM_ERROR, EDIT_DISTANCE_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT) {
        strncpy(message, EDIT_DISTANCE_ARG_TYPE_ERROR, EDIT_DISTANCE_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to allocate a buffer.
    initid->ptr = (char *)new(std::nothrow) std::vector<size_t>((EDIT_DISTANCE_MAX_EDIT_DIST));
    if (initid->ptr == nullptr) {
        strncpy(message, EDIT_DISTANCE_MEM_ERROR, EDIT_DISTANCE_MEM_ERROR_LEN);
        return 1;
    }

    // damlev2D does not return null.
    initid->maybe_null = 0;
    return 0;
}

void damlev2D_deinit(UDF_INIT *initid) {
    delete[] initid->ptr;
}

long long damlev2D(UDF_INIT *initid, UDF_ARGS *args, UNUSED char *is_null, UNUSED char *error) {

    std::string_view S1{args->args[0], args->lengths[0]};
    std::string_view S2{args->args[1], args->lengths[1]};

    //https://takeuforward.org/data-structure/edit-distance-dp-33/
    int n = S1.size();
    int m = S2.size();

    std::vector<int> prev(m+1,0);
    std::vector<int> cur(m+1,0);

    int cost;
    for(int j=0;j<=m;j++){
        prev[j] = j;
    }

    for(int i=1;i<n+1;i++){
        cur[0]=i;

        for(int j=1;j<m+1;j++){
            if(S1[i-1]==S2[j-1]){
                cur[j] = 0+prev[j-1];
                cost = 0;
            }

            else {
                cur[j] = 1 + std::min(prev[j - 1], std::min(prev[j], cur[j - 1]));
                cost = 1;
            }
            if( (i > 1) &&
                (j > 1) &&
                (S1[i-1] == S2[j-2]) &&
                (S1[i-2] == S2[j-1])
                    )
            {
                cur[j] = std::min(
                        cur[j],
                        prev[j-1]  +cost  // transposition
                );
            }
        } prev = cur;
    }

    return prev[m];
}