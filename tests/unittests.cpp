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
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});

            if (i > 1 && j > 1 && S1[i - 1] == S2[j - 2] && S1[i - 2] == S2[j - 1]) {
                dp[i][j] = std::min(dp[i][j], dp[i - 2][j - 2] + cost);
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
    return getRandomInt(1, std::min(static_cast<int>(str.length()), 5));
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

// Helper function for running test cases
void runTestCase(const TestCase& testCase) {
    long long result = LEV_CALL(
            const_cast<char*>(testCase.a.c_str()),
            testCase.a.size(),
            const_cast<char*>(testCase.b.c_str()),
            testCase.b.size(),
            3 // Assuming a max distance of 3
    );
    ASSERT_EQ(result, testCase.expectedDistance) << "Test case: " << testCase.functionName << " failed.";
}

// Maximum number of allowed failures before breaking the loop
const int MAX_FAILURES = 1;

// Specific test for Transposition
TEST_F(LevenshteinTest, Transposition) {
    int failureCount = 0;
    for (int i = 0; i < LOOP; ++i) {
        std::string original = getRandomString(wordList);
        int editCount = getRandomEditCount(original);
        std::string modified = applyTransposition(original, editCount);
        TestCase testCase = {original, modified, calculateDamLevDistance(original, modified), "Transposition"};

        long long result = LEV_CALL(
                const_cast<char*>(testCase.a.c_str()),
                testCase.a.size(),
                const_cast<char*>(testCase.b.c_str()),
                testCase.b.size(),
                3 // Assuming a max distance of 3
        );

        if (result != testCase.expectedDistance) {
            failureCount++;
            if (failureCount >= MAX_FAILURES) {
                ADD_FAILURE() << "Transposition test failed for " << failureCount << " cases. Breaking loop.";
                ADD_FAILURE() << "Transposition test failed " << testCase.a<< " -- "<<testCase.b <<" "<<testCase.expectedDistance<<" vs. "<<result;
                break;
            }
        }
    }
    if (failureCount > 0) {
        FAIL() << "Transposition test failed for " << failureCount << " cases.";
    }
}

// Specific test for Deletion
TEST_F(LevenshteinTest, Deletion) {
    int failureCount = 0;
    for (int i = 0; i < LOOP; ++i) {
        std::string original = getRandomString(wordList);
        int editCount = getRandomEditCount(original);
        std::string modified = applyDeletion(original, editCount);
        TestCase testCase = {original, modified, calculateDamLevDistance(original, modified), "Deletion"};

        long long result = LEV_CALL(
                const_cast<char*>(testCase.a.c_str()),
                testCase.a.size(),
                const_cast<char*>(testCase.b.c_str()),
                testCase.b.size(),
                3 // Assuming a max distance of 3
        );

        if (result != testCase.expectedDistance) {
            failureCount++;
            if (failureCount >= MAX_FAILURES) {
                ADD_FAILURE() << "Deletion test failed for " << failureCount << " cases. Breaking loop.";
                break;
            }
        }
    }
    if (failureCount > 0) {
        FAIL() << "Deletion test failed for " << failureCount << " cases.";
    }
}

// Specific test for Insertion
TEST_F(LevenshteinTest, Insertion) {
    int failureCount = 0;
    for (int i = 0; i < LOOP; ++i) {
        std::string original = getRandomString(wordList);
        int editCount = getRandomEditCount(original);
        std::string modified = applyInsertion(original, editCount);
        TestCase testCase = {original, modified, calculateDamLevDistance(original, modified), "Insertion"};

        long long result = LEV_CALL(
                const_cast<char*>(testCase.a.c_str()),
                testCase.a.size(),
                const_cast<char*>(testCase.b.c_str()),
                testCase.b.size(),
                3 // Assuming a max distance of 3
        );

        if (result != testCase.expectedDistance) {
            failureCount++;
            if (failureCount >= MAX_FAILURES) {
                ADD_FAILURE() << "Insertion test failed for " << failureCount << " cases. Breaking loop.";
                break;
            }
        }
    }
    if (failureCount > 0) {
        FAIL() << "Insertion test failed for " << failureCount << " cases.";
    }
}

// Specific test for Substitution
TEST_F(LevenshteinTest, Substitution) {
    int failureCount = 0;
    for (int i = 0; i < LOOP; ++i) {
        std::string original = getRandomString(wordList);
        int editCount = getRandomEditCount(original);
        std::string modified = applySubstitution(original, editCount);
        TestCase testCase = {original, modified, calculateDamLevDistance(original, modified), "Substitution"};

        long long result = LEV_CALL(
                const_cast<char*>(testCase.a.c_str()),
                testCase.a.size(),
                const_cast<char*>(testCase.b.c_str()),
                testCase.b.size(),
                3 // Assuming a max distance of 3
        );

        if (result != testCase.expectedDistance) {
            failureCount++;
            if (failureCount >= MAX_FAILURES) {
                ADD_FAILURE() << "Substitution test failed for " << failureCount << " cases. Breaking loop.";
                break;
            }
        }
    }
    if (failureCount > 0) {
        FAIL() << "Substitution test failed for " << failureCount << " cases.";
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
