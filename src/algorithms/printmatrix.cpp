#pragma once
#include <iostream>

void printMatrix(const int* dp, int n, int m, const std::string_view& S1, const std::string_view& S2) {
    // Print the column headers
    std::cout << "  ";
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
            // Access the element using the formula: dp[i * (m + 1) + j]
            std::cout << dp[i * (m + 1) + j] << " ";
        }
        std::cout << std::endl;
    }
}
