// unittest_gtest.cpp
#include <gtest/gtest.h>
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

int LOOP = 100000; // Adjust as necessary for testing speed
std::vector<TestCase> failedTests; // Vector to store failed test cases

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

// Helper function to check if a value is within a given range.
::testing::AssertionResult IsBetweenInclusive(int val, int lower_bound, int upper_bound) {
    if ((val >= lower_bound) && (val <= upper_bound))
        return ::testing::AssertionSuccess();
    else
        return ::testing::AssertionFailure() << val << " is outside the range " << lower_bound << " to " << upper_bound;
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

int getRandomEditCount(const std::string& str) {
    if (str.empty()) return 0;
    return getRandomInt(1, std::min(static_cast<int>(str.length()), 5)); // Limiting to a max of 5 for practicality
}

std::string applyTransposition(std::string str, int editCount) {
    for (int i = 0; i < editCount; ++i) {
        if (str.length() < 2) break;
        int pos = getRandomInt(0, str.length() - 2);
        std::swap(str[pos], str[pos + 1]);
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

// Fixture class for Google Test
class LevenshteinTest : public ::testing::Test {
protected:
    std::vector<std::string> wordList;

    void SetUp() override {
        std::string primaryFilePath = "tests/taxanames";
        std::string fallbackFilePath = WORDS_PATH;
        unsigned maximum_size = WORD_COUNT;

        boost::interprocess::file_mapping text_file;
        boost::interprocess::mapped_region text_file_buffer;

        try {
            text_file = boost::interprocess::file_mapping(primaryFilePath.c_str(), boost::interprocess::read_only);
            text_file_buffer = boost::interprocess::mapped_region(text_file, boost::interprocess::read_only);
        } catch (const boost::interprocess::interprocess_exception&) {
            try {
                text_file = boost::interprocess::file_mapping(fallbackFilePath.c_str(), boost::interprocess::read_only);
                text_file_buffer = boost::interprocess::mapped_region(text_file, boost::interprocess::read_only);
            } catch (const boost::interprocess::interprocess_exception&) {
                FAIL() << "Could not open fallback file " << fallbackFilePath;
            }
        }

        wordList = readWordsFromMappedFile(text_file_buffer, maximum_size);
        LEV_SETUP();
    }

    void TearDown() override {
        LEV_TEARDOWN();
    }
};



// Maximum number of allowed failures before breaking the loop
const int MAX_FAILURES = 1;

template <typename Func>
void RunLevenshteinTest(const char* function_name, Func applyEditFunc, const std::vector<std::string>& wordList, int max_distance) {
    int failureCount = 0;
    for (int i = 0; i < LOOP; ++i) {
        std::string original = getRandomString(wordList);
        int editCount = getRandomEditCount(original);
        std::string modified = applyEditFunc(original, editCount);
        TestCase testCase = {original, modified, calculateDamLevDistance(original, modified), function_name};

        long long result = LEV_CALL(
                const_cast<char*>(testCase.a.c_str()),
                testCase.a.size(),
                const_cast<char*>(testCase.b.c_str()),
                testCase.b.size(),
                max_distance // Assuming a max distance of 3
        );

        // Determine bounds based on the current algorithm being tested
        const char* algorithm_name = LEV_ALGORITHM_NAME;
        int lower_bound, upper_bound;

        if (strcmp(algorithm_name, "damlevlim") == 0 || strcmp(algorithm_name, "damlevmin") == 0) {
            lower_bound = std::min(testCase.expectedDistance, max_distance + 1);
            upper_bound = std::max(testCase.expectedDistance, max_distance + 1);
        } else {
            lower_bound = testCase.expectedDistance;
            upper_bound = testCase.expectedDistance;
        }

        if (!IsBetweenInclusive(result, lower_bound, upper_bound)) {
            failureCount++;
            if (failureCount >= MAX_FAILURES) {
                ADD_FAILURE() << function_name << " test failed for " << failureCount << " cases. Breaking loop.";
                ADD_FAILURE() << function_name << " test failed " << testCase.a << " -- " << testCase.b << " " << testCase.expectedDistance << " vs. " << result;
                break;
            }
        }
    }
    if (failureCount > 0) {
        FAIL() << function_name << " test failed for " << failureCount << " cases.";
    }
}


//Here we should get each function we want to test and loop through that AVAILABLE_ALGORITHMS
//is it possible to do that with LEV_CALL?  We use LEV_CALL in one_off testing as well.
// Specific test for Transposition
TEST_F(LevenshteinTest, Transposition) {
    RunLevenshteinTest("Transposition", applyTransposition, wordList, 3);
}

// Specific test for Deletion
TEST_F(LevenshteinTest, Deletion) {
    RunLevenshteinTest("Deletion", applyDeletion, wordList,3);
}

// Specific test for Insertion
TEST_F(LevenshteinTest, Insertion) {
    RunLevenshteinTest("Insertion", applyInsertion, wordList,3);
}

// Specific test for Substitution
TEST_F(LevenshteinTest, Substitution) {
    RunLevenshteinTest("Substitution", applySubstitution, wordList,3);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
