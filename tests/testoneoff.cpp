// HELLO LEV_ARGS needs to be adjusted if you're going to change from damlevconst
//damlevconst INT required testharness.hpp --> LEV_ARGS->arg_count = 3;
// damlev testharness.hpp --> LEV_ARGS->arg_count = 2;
//damlevlimp FLOAT required testharness.hpp LEV_ARGS->arg_count = 3 A maximum percent difference (0.0 <= float < 1.0);
#define PRINT_DEBUG
#include <iostream>
#include <string>

#define xstr(s) str(s)
#define str(s) #s

#include "testharness.hpp"

std::string search_term {"erecta"};
std::string db_return {  "rceaet"};

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

int calculateDamLevDistance(const std::string& S1_input, const std::string& S2_input, int max) {
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
            if (i > 1 && j > 1 && S1[i - 1] == S2[j - 2] && S1[i - 2] == S2[j - 1]) {
                dp[i][j] = std::min(dp[i][j], dp[i - 2][j - 2] + cost); // transposition
            }

            column_min = std::min(column_min, dp[i][j]);
        }


        // Print matrix after processing each row
        printMatrix(dp, S1, S2);

        // Early exit if the minimum edit distance in this row exceeds effective_max
        if (column_min > effective_max) {
            std::cout << "Early exit: minimum distance " << column_min << " exceeds effective_max " << effective_max << std::endl;
            return effective_max + 1; // Return effective_max + 1 on early exit
        }
    }

    return dp[n][m];
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    std::string a = search_term;
    std::string b = db_return;
    long long c =3;
    long long result = 0;
    long long result2 = 0;
    long long result3 = 0;
    long long result4 = 0;


    LEV_SETUP();
    std::cout << "Subject is Search Term: " <<  std::endl;

    result = LEV_CALL(const_cast<char*>(a.c_str()),
                      a.size(),
                      const_cast<char*>(b.c_str()),
                      b.size(),
                      c
                      );

    std::cout << xstr(LEV_FUNCTION) "(" << a << ", " << b << ") =  " << result << std::endl;

    std::cout << "Query is Search Term: " <<  std::endl;

    result2 = LEV_CALL(const_cast<char*>(b.c_str()),
                      b.size(),
                      const_cast<char*>(a.c_str()),
                      a.size(),
                      c
    );

    std::cout << xstr(LEV_FUNCTION) "(" << a << ", " << b << ") =  " << result2 << std::endl;


    LEV_TEARDOWN();

    result3 = calculateDamLevDistance(a, b, c);
    std::cout << xstr(calculateDamLevDistance) "(" << a << ", " << b << ") =  " << result3 << std::endl;
    result4 = calculateDamLevDistance(b, a, c);
    std::cout << xstr(calculateDamLevDistance) "(" << a << ", " << b << ") =  " << result4 << std::endl;

}


