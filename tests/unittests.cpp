#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include "testharness.hpp"

// Random Number Generator
std::random_device rd;
std::mt19937 gen(rd());

struct TestCase {
    std::string a;
    std::string b;
    int expectedDistance;
    std::string functionName;
};

long long calculateDamLevDistance(const std::string& S1, const std::string& S2) {
    int n = S1.size();
    int m = S2.size();

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
            dp[i][j] = std::min({ dp[i - 1][j] + 1, // Deletion
                                  dp[i][j - 1] + 1, // Insertion
                                  dp[i - 1][j - 1] + cost }); // Substitution

            if (i > 1 && j > 1 && S1[i - 1] == S2[j - 2] && S1[i - 2] == S2[j - 1]) {
                dp[i][j] = std::min(dp[i][j], dp[i - 2][j - 2] + cost); // Transposition
            }
        }
    }

    return dp[n][m];
}

int getRandomInt(int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

char getRandomChar() {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    return chars[dis(gen)];
}

void swapRandomChars(std::string& str) {
    if (str.length() < 2) return;
    int i = getRandomInt(0, str.length() - 1);
    int j = getRandomInt(0, str.length() - 1);
    std::swap(str[i], str[j]);
}

int getRandomEditCount(const std::string& str) {
    if (str.empty()) return 0;
    return getRandomInt(1, std::min(static_cast<int>(str.length()), 5)); // Limiting to a max of 5 for practicality
}

std::string applyTransposition(std::string str, int editCount) {
    for (int i = 0; i < editCount; ++i) {
        if (str.length() < 2) break; // Can't transpose a string with fewer than 2 characters
        int pos = getRandomInt(0, str.length() - 2); // Select a position where there is a character to swap with
        std::swap(str[pos], str[pos + 1]); // Swap with the next character
    }
    return str;
}

std::string applyDeletion(std::string str, int editCount) {
    for (int i = 0; i < editCount; ++i) {
        if (!str.empty()) {
            int pos = getRandomInt(0, str.length() - 1);
            str.erase(pos, 1);
        }
    }
    return str;
}

std::string applyInsertion(const std::string& str, int editCount) {
    std::string result = str;
    for (int i = 0; i < editCount; ++i) {
        int pos = getRandomInt(0, result.length());
        char ch = getRandomChar();
        result.insert(pos, 1, ch);
    }
    return result;
}

std::string applySubstitution(const std::string& str, int editCount) {
    std::string result = str;
    for (int i = 0; i < editCount; ++i) {
        if (!result.empty()) {
            int pos = getRandomInt(0, result.length() - 1);
            result[pos] = getRandomChar();
        }
    }
    return result;
}

std::string getRandomString(const std::vector<std::string>& wordList) {
    if (wordList.empty()) return "";
    int index = getRandomInt(0, wordList.size() - 1);
    return wordList[index];
}

// ...

int main() {
    std::vector<std::string> wordList = {"word", "paragraph", /* ... */ };
    std::vector<TestCase> testCases = {
            // Your predefined test cases
            {"test", "test", 0, "Identical"}, // Identical strings
            // ... (Other predefined test cases) ...
    };

    // Generate random test cases for each type
    for (int i = 0; i < 100; ++i) {
        std::string a = getRandomString(wordList);
        std::string b;
        int editCount;

        // Transposition
        editCount = getRandomEditCount(a);
        b = applyTransposition(a, editCount);
        testCases.push_back({a, b, static_cast<int>(calculateDamLevDistance(a, b)), "Transposition"});

        // Deletion
        editCount = getRandomEditCount(a);
        b = applyDeletion(a, editCount);
        testCases.push_back({a, b, static_cast<int>(calculateDamLevDistance(a, b)), "Deletion"});

        // Insertion
        editCount = getRandomEditCount(a);
        b = applyInsertion(a, editCount);
        testCases.push_back({a, b, static_cast<int>(calculateDamLevDistance(a, b)), "Insertion"});

        // Substitution
        editCount = getRandomEditCount(a);
        b = applySubstitution(a, editCount);
        testCases.push_back({a, b, static_cast<int>(calculateDamLevDistance(a, b)), "Substitution"});
    }




    long long maxDistance = 9; // Maximum edit distance
    LEV_SETUP();

    // Run tests
    for (const auto& testCase : testCases) {
        long long result = LEV_CALL(const_cast<char*>(testCase.a.c_str()), testCase.a.size(),
                                    const_cast<char*>(testCase.b.c_str()), testCase.b.size(),
                                    maxDistance);

        bool testPassed = result == testCase.expectedDistance;
        std::cout << testCase.functionName << ": " << testCase.a << " vs " << testCase.b
                  << " (LD: " << result << ") - " << " (Expected: " << testCase.expectedDistance << ") - "
                  << (testPassed ? "PASS" : "FAIL") << std::endl;
    }

    LEV_TEARDOWN();
}