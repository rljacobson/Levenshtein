//unittest.cpp
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <sstream>

#ifndef WORD_COUNT
#define WORD_COUNT 10000ul
#endif
#ifndef WORDS_PATH
#define WORDS_PATH "/usr/share/dict/words"
#endif

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

int LOOP = 100000;
int FAILED = 0;

std::vector<std::string> readWordsFromMappedFile(const boost::interprocess::mapped_region& region, unsigned maximumWords) {
    const char* begin = static_cast<const char*>(region.get_address());
    const char* end = begin + region.get_size();
    std::string fileContent(begin, end);

    std::istringstream iss(fileContent);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word && words.size() < maximumWords) {
        words.push_back(word);
    }

    return words;
}

int calculateDamLevDistance(const std::string& S1, const std::string& S2) {
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

std::string getRandomStringOfLength(int length) {
    std::string result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result += getRandomChar(); // Reuse the existing getRandomChar function
    }
    return result;
}

std::string getRandomString(const std::vector<std::string>& wordList) {
    if (wordList.empty()) return "";
    int index = getRandomInt(0, wordList.size() - 1);
    return wordList[index];
}
std::string applyCommonPrefix(std::string str, const std::string& prefix) {
    return prefix + str;
}

std::string applyCommonSuffix(std::string str, const std::string& suffix) {
    return str + suffix;
}

std::string applyBothPrefixAndSuffix(std::string str, const std::string& prefix, const std::string& suffix) {
    return prefix + str + suffix;
}


// ...

int main() {
    std::string primaryFilePath = "tests/taxanames";  // Primary file path
    std::string fallbackFilePath = WORDS_PATH;  // Fallback file path
    unsigned maximum_size = WORD_COUNT;

    boost::interprocess::file_mapping text_file;
    boost::interprocess::mapped_region text_file_buffer;  // Corrected declaration

    try {
        text_file = boost::interprocess::file_mapping(primaryFilePath.c_str(), boost::interprocess::read_only);
        text_file_buffer = boost::interprocess::mapped_region(text_file,
                                                              boost::interprocess::read_only);  // Initialization
        std::cout << "Using word list " << primaryFilePath << "\n";
    } catch (const boost::interprocess::interprocess_exception &ePrimary) {
        try {
            text_file = boost::interprocess::file_mapping(fallbackFilePath.c_str(), boost::interprocess::read_only);
            text_file_buffer = boost::interprocess::mapped_region(text_file,
                                                                  boost::interprocess::read_only);  // Initialization
            std::cout << "Using word list " << fallbackFilePath << "\n";
        } catch (const boost::interprocess::interprocess_exception &eFallback) {
            std::cerr << "Could not open fallback file " << fallbackFilePath << '.' << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::vector<std::string> wordList = readWordsFromMappedFile(text_file_buffer, maximum_size);

    std::vector<TestCase> testCases = {
            // Tests have the form:
            // { original string, modified string, expected distance, name of test }
            {"test", "test", 0, "Identical"}, // Identical strings
    };

    // Generate random test cases for each type
    for (int i = 0; i < LOOP; ++i) {
        std::string original = getRandomString(wordList); // Original string
        std::string modified; // String after applying edit operation
        int editCount;

        // Transposition
        editCount = getRandomEditCount(original);
        modified = applyTransposition(original, editCount);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Transposition"});

        // Deletion
        editCount = getRandomEditCount(original);
        modified = applyDeletion(original, editCount);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Deletion"});

        // Insertion
        editCount = getRandomEditCount(original);
        modified = applyInsertion(original, editCount);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Insertion"});

        // Substitution
        editCount = getRandomEditCount(original);
        modified = applySubstitution(original, editCount);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Substitution"});


        // Generate random prefix and suffix of length up to 10
        std::string commonPrefix = getRandomStringOfLength(getRandomInt(1, 10));
        std::string commonSuffix = getRandomStringOfLength(getRandomInt(1, 10));

        // Tests with Common Prefix
        editCount = getRandomEditCount(original);
        modified = applyCommonPrefix(applyTransposition(original, editCount), commonPrefix);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Common Prefix"});

        // Tests with Common Prefix
        editCount = getRandomEditCount(original);
        modified = applyCommonPrefix(applyDeletion(original, editCount), commonPrefix);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Common Prefix"});

        // Tests with Common Suffix
        editCount = getRandomEditCount(original);
        modified = applyCommonSuffix(applyDeletion(original, editCount), commonSuffix);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Common Suffix"});

        // Tests with Common Suffix
        editCount = getRandomEditCount(original);
        modified = applyCommonSuffix(applyInsertion(original, editCount), commonSuffix);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Common Suffix"});

        // Tests with Both Common Prefix and Suffix
        editCount = getRandomEditCount(original);
        modified = applyBothPrefixAndSuffix(applyInsertion(original, editCount), commonPrefix, commonSuffix);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Prefix and Suffix"});
        // Tests with Both Common Prefix and Suffix
        editCount = getRandomEditCount(original);
        modified = applyBothPrefixAndSuffix(applyTransposition(original, editCount), commonPrefix, commonSuffix);
        testCases.push_back({original, modified, calculateDamLevDistance(original, modified), "Prefix and Suffix"});

    }

    auto total_start = std::chrono::high_resolution_clock::now();

    // Run all test cases
    LEV_SETUP();
    for (const auto& testCase : testCases) {
        long long result
            = LEV_CALL(
                const_cast<char*>(testCase.a.c_str()),
                testCase.a.size(),
                const_cast<char*>(testCase.b.c_str()),
                testCase.b.size(),
                25 // Assuming a max distance of 25
            );
        bool testPassed = result == testCase.expectedDistance;
        if (!testPassed) { // Print only if the test failed
            std::cout << testCase.functionName << ": " << testCase.a << " vs " << testCase.b
                      << " (LD: " << result << ") - " << " (Expected: " << testCase.expectedDistance << ") - FAIL"
                      << std::endl;
            FAILED++;
            if (FAILED > 25) {
                LEV_TEARDOWN();  // Tear down after test case
                std::cout << "FAILED FOR > 25 STRINGS" << std::endl;
                return 99;
            }
        }

    }
    LEV_TEARDOWN();

    // Stop the timer after running all test cases
    auto total_stop = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_stop - total_start);

    if (FAILED < 1) {
        std::cout << "ALL PASSED FOR " << LOOP << " STRINGS" << std::endl;
    } else {
        std::cout << "FAILED FOR " << FAILED << " STRINGS" << std::endl;
    }

    // Print the total time for all tests
    std::cout << "Total time elapsed for all test cases: " << total_duration.count() << " milliseconds" << std::endl;

    return 0;
}


