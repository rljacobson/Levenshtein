#define PRINT_DEBUG
#define CAPTURE_METRICS
#include <iostream>
#include <string>


// region Algorithms Setup

// The two algorithms will be referred to as ALGORITHM_A and ALGORITHM_B. We
// need to capture four symbols in each case:
//   1. `LEV_SETUP`
//   2. `LEV_TEARDOWN`
//   3. `LEV_CALL`
//   4. `LEV_ALGORITHM_NAME`
//
// We capture them in the symbols `ALGORITHM_X_SETUP`, `ALGORITHM_X_TEARDOWN`,
// `ALGORITHM_X_CALL`, and `ALGORITHM_X_NAME` respectively, where `X` stands
// in for `A` and `B`.

#ifdef ALGORITHM_A
// include for ALGORITHM_A
#define LEV_FUNCTION ALGORITHM_A
#define LEV_ALGORITHM_COUNT ALGORITHM_A_COUNT
#include "testharness.hpp"
// capture symbols for ALGORITHM_A
void (* const algorithm_a_setup)() = LEV_SETUP;
void (* const algorithm_a_teardown)() = LEV_TEARDOWN;
long long (* const algorithm_a_call)(char*, size_t, char*, size_t, long long) = LEV_CALL;
char * algorithm_a_name = LEV_ALGORITHM_NAME;

// Now include for ALGORITHM_B
#ifdef ALGORITHM_B
#define LEV_FUNCTION ALGORITHM_B
#define LEV_ALGORITHM_COUNT ALGORITHM_B_COUNT
#else
// The "default" comparison is to plain vanilla damlev.
#define LEV_FUNCTION damlev2D
// Keep in synch with LEV_FUNCTION default above.
#define LEV_ALGORITHM_COUNT 2
#endif
#include "testharness.hpp"
// capture symbols for ALGORITHM_B
#define ALGORITHM_B_SETUP LEV_SETUP
#define ALGORITHM_B_TEARDOWN LEV_TEARDOWN
#define ALGORITHM_B_CALL LEV_CALL
#define ALGORITHM_B_NAME LEV_ALGORITHM_NAME

#else
// Assume client code has already specified `LEV_FUNCTION` and `LEV_ALGORITHM_COUNT`.
#include "testharness.hpp"
// capture symbols for ALGORITHM_A
void (* const algorithm_a_setup)() = LEV_SETUP;
void (* const algorithm_a_teardown)() = LEV_TEARDOWN;
long long (* const algorithm_a_call)(char*, size_t, char*, size_t, long long) = LEV_CALL;
char * algorithm_a_name = LEV_ALGORITHM_NAME;

// The "default" comparison is to plain vanilla damlev2D.
#define LEV_FUNCTION damlev2D
// Keep in synch with LEV_FUNCTION default above.
#define LEV_ALGORITHM_COUNT 2
#include "testharness.hpp"
// capture symbols for ALGORITHM_B
#define ALGORITHM_B_SETUP LEV_SETUP
#define ALGORITHM_B_TEARDOWN LEV_TEARDOWN
#define ALGORITHM_B_CALL LEV_CALL
#define ALGORITHM_B_NAME LEV_ALGORITHM_NAME

#endif

// endregion

std::string string_a {"brevifrontalis"};
std::string string_b {"brefontals"};

void printMatrix(const std::vector<std::vector<int>>& dp, const std::string& S1, const std::string& S2) {
    int n = S1.size();
    int m = S2.size();

    // Print the column headers
    std::cout << "    ";
    for (int j = 0; j <= m; ++j) {
        if (j == 0) {
            std::cout << "  ";
        } else {
            std::cout << S2[j - 1] << " ";
        }
    }
    std::cout << std::endl;

    // Print the matrix with row headers
    for (int i = 0; i <= n; ++i) {
        if (i == 0) {
            std::cout << "  ";
        } else {
            std::cout << S1[i - 1] << " ";
        }

        for (int j = 0; j <= m; ++j) {
            std::cout << dp[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

// This algorithm is "plain vanilla" edit distance. The only optimization it supports is
// a maximum disntace
int calculateLevDistance(const std::string& S1_input, const std::string& S2_input, int max) {
    std::string S1 = S1_input;
    std::string S2 = S2_input;

    // Ensure S1 is the shorter string
    if (S2.size() < S1.size()) {
        std::swap(S1, S2);
    }

    int n = S1.size();
    int m = S2.size();

    // Calculate effective_max using the length of the shorter string
    int effective_max = std::min(max, n);

    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    // Initialize dp table
    for (int i = 0; i <= n; i++) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= m; j++) {
        dp[0][j] = j;
    }

    // Main loop to calculate the Damerau-Levenshtein distance
    for (int i = 1; i <= n; i++) {
        int column_min = INT_MAX; // Initialize column_min for early exit check

        for (int j = 1; j <= m; j++) {
            int cost = (S1[i - 1] == S2[j - 1]) ? 0 : 1;

            dp[i][j] = std::min({dp[i - 1][j] + 1,  // deletion
                                 dp[i][j - 1] + 1,  // insertion
                                 dp[i - 1][j - 1] + cost}); // substitution

            // Check for transpositions
            // if (i > 1 && j > 1 && S1[i - 1] == S2[j - 2] && S1[i - 2] == S2[j - 1]) {
            //     dp[i][j] = std::min(dp[i][j], dp[i - 2][j - 2] + cost); // transposition
            // }

            column_min = std::min(column_min, dp[i][j]);
        }


        // Print matrix after processing each row
        // printMatrix(dp, S1, S2);

        // Early exit if the minimum edit distance in this row exceeds effective_max
        if (column_min > effective_max) {
            std::cout << "Early exit: minimum distance " << column_min << " exceeds effective_max " << effective_max << std::endl;
            return effective_max + 1; // Return effective_max + 1 on early exit
        }
    }

    return dp[n][m];
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    initialize_metrics();

    int max_distance = 5;

    // We use `ALGORITHM_B` as the "expected" value, although this is somewhat arbitrary.
    ALGORITHM_B_SETUP();
    long long expected_distance = ALGORITHM_B_CALL(
            const_cast<char*>(string_a.c_str()),
            string_a.size(),
            const_cast<char*>(string_b.c_str()),
            string_b.size(),
            max_distance
    );
    ALGORITHM_B_TEARDOWN();

    // We consider `ALGORITHM_A` the algorithm under test, although this is somewhat arbitrary.
    algorithm_a_setup();
    long long result = algorithm_a_call(
            const_cast<char*>(string_a.c_str()),
            string_a.size(),
            const_cast<char*>(string_b.c_str()),
            string_b.size(),
            max_distance
    );
    algorithm_a_teardown();

    int actual_distance = calculateLevDistance(string_a, string_b, max_distance);

    std::cout << "string_a: " << string_a << "\n";
    std::cout << "string_b: " << string_b << "\n";
    std::cout << "max: " << max_distance << "\n";
    std::cout << algorithm_a_name << ": " << result << "\n";
    std::cout << ALGORITHM_B_NAME << ": " << expected_distance << "\n";
    std::cout << "Actual: " << actual_distance << "\n";
    print_metrics();
}


