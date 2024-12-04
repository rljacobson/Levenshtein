/*
Copyright (C) 2019 Robert Jacobson
Distributed under the MIT License. See License.txt for details.

<hr>

`DAMLEV2D(String1, String2)`

Computes the Damarau-Levenshtein edit distance between two strings. This
function should not be used. It exists for testing and benchmarking purposes
only.

Syntax:

    DAMLEV2D(String1, String2);

`String1`:  A string constant or column.
`String2`:  A string constant or column to be compared to `String1`.

Returns: An integer equal to the Damarau-Levenshtein edit distance between
`String1` and `String2`.

Example Usage:

    SELECT Name, DAMLEV2D(Name, "Vladimir Iosifovich Levenshtein") AS EditDist
        FROM CUSTOMERS
        WHERE DAMLEV2D(Name, "Vladimir Iosifovich Levenshtein") < 6;

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".

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


UDF_SIGNATURES(damlev2D)


[[maybe_unused]]
int damlev2D_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 2 arguments:
    if (args->arg_count != 2) {
        strncpy(message, EDIT_DISTANCE_ARG_NUM_ERROR, EDIT_DISTANCE_ARG_NUM_ERROR_LEN);
        return int(1);
    }
        // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT) {
        strncpy(message, EDIT_DISTANCE_ARG_TYPE_ERROR, EDIT_DISTANCE_ARG_TYPE_ERROR_LEN);
        return int(1);
    }

    // Attempt to allocate a buffer.
    initid->ptr = (char *)new(std::nothrow) std::vector<size_t>((EDIT_DISTANCE_MAX_EDIT_DIST));
    if (initid->ptr == nullptr) {
        strncpy(message, EDIT_DISTANCE_MEM_ERROR, EDIT_DISTANCE_MEM_ERROR_LEN);
        return int(1);
    }

    // damlev2D does not return null.
    initid->maybe_null = 0;
    return int(0);
}

[[maybe_unused]]
void damlev2D_deinit(UDF_INIT *initid) {
    delete[] initid->ptr;
}

[[maybe_unused]]
long long damlev2D(UDF_INIT *, UDF_ARGS *args, [[maybe_unused]]  char *is_null, [[maybe_unused]]  char *error) {

    std::string_view S1{args->args[0], args->lengths[0]};
    std::string_view S2{args->args[1], args->lengths[1]};

    int n = static_cast<int>(S1.size());
    int m = static_cast<int>(S2.size());

    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    for (int i = 0; i <= n; i++) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= m; j++) {
        dp[0][j] = j;
    }

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            int cost = (S1[i - 1] == S2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, // Deletion
                                 dp[i][j - 1] + 1, // Insertion
                                 dp[i - 1][j - 1] + cost}); // Substitution

            if (i > 1 && j > 1 && S1[i - 1] == S2[j - 2] && S1[i - 2] == S2[j - 1]) {
                dp[i][j] = std::min(dp[i][j], dp[i - 2][j - 2] + cost); // Transposition
            }
        }
    }

    return dp[n][m];
}
