/**
There are two ways to use this file.

1. Define `LEV_FUNCTION`, which is the unquoted algorithm name, and `LEV_ALGORITHM_COUNT`,
    the number of arguments the function takes. This code will then compare the output of the
    algorithm to the plain damlev2D algorithm. (The defined `LEV_FUNCTION` will be `ALGORITHM_A`,
    and damlev2D will be `ALGORITHM_B`.)

2. Define `ALGORITHM_A` and `ALGORITHM_A_COUNT`, the unquoted algorithm name and argument
    count respectively of the first algorithm to compare, and likewise `ALGORITHM_B` and
    `ALGORITHM_B_COUNT` for the second algorithm to compare. This code will then compare the
    outputs of these two algorithms.

*/
#define CAPTURE_METRICS

#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <sstream>

#include "edit_operations.hpp"
#include "metrics.hpp"

#ifndef WORDS_PATH
#define WORDS_PATH "/usr/share/dict/words"
#endif

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

struct TestCase {
    std::string a;
    std::string b;
    long long expectedDistance;
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

// Fixture class for Google Test
class LevenshteinTest : public ::testing::Test {
protected:
    std::vector<std::string> wordList;

    /**
    Sets up the test fixture by initializing the word list used in the tests.

    This function is called before each test in the LevenshteinTest class.
    It attempts to open a primary file containing words from the path
    "tests/taxanames". If the primary file cannot be opened, it falls back
    to a predefined fallback file path specified by WORDS_PATH. Both file
    accesses are done using Boost's interprocess library to map the file
    contents into memory for efficient reading.

    If both file openings fail, the test will fail with an error message
    indicating that the fallback file could not be opened. Upon successful
    file access, the words are read into the `wordList` vector, limited by
    the maximum size defined by word_count. This word list is then used
    in the tests for string manipulation and comparison operations related
    to the Levenshtein distance algorithm.
    */
    void SetUp() override {
        /*

        std::string primaryFilePath = "tests/taxanames";
        std::string fallbackFilePath = WORDS_PATH;
        unsigned maximum_size = 200000;

        boost::interprocess::file_mapping text_file;
        boost::interprocess::mapped_region text_file_buffer;

        try {
            text_file = boost::interprocess::file_mapping(primaryFilePath.c_str(), boost::interprocess::read_only);
            text_file_buffer = boost::interprocess::mapped_region(text_file, boost::interprocess::read_only);
            std::cout << "Using words from " << primaryFilePath << std::endl;
        } catch (const boost::interprocess::interprocess_exception&) {
            try {
                text_file = boost::interprocess::file_mapping(fallbackFilePath.c_str(), boost::interprocess::read_only);
                text_file_buffer = boost::interprocess::mapped_region(text_file, boost::interprocess::read_only);
                std::cout << "Using words from " << fallbackFilePath << std::endl;
            } catch (const boost::interprocess::interprocess_exception&) {
                FAIL() << "Could not open fallback file " << fallbackFilePath;
            }
        }
        wordList = readWordsFromMappedFile(text_file_buffer, maximum_size);
        */

        int word_count = 200000;
        int word_length = 40;
        wordList.reserve(word_count);

        auto gen_word = [word_length](){return generateRandomString(word_length);};

        std::generate_n(std::back_inserter(wordList), word_count, gen_word); // Fill the string

    }

};

void PrintFailedTestCase(const TestCase& testCase, long long actualResult) {
    std::cout << "Test Case Failed:" << std::endl;
    std::cout << "-------------------------------------" << std::endl;
    std::cout << "Original String: " << testCase.a << std::endl;
    std::cout << "Modified String: " << testCase.b << std::endl;
    std::cout << ALGORITHM_B_NAME << " Distance: " << testCase.expectedDistance << std::endl;
    std::cout << algorithm_a_name << " Result: " << actualResult << std::endl;
    std::cout << "Function Name: " << testCase.functionName << std::endl;
    std::cout << "-------------------------------------" << std::endl;
}

// Maximum number of allowed failures before breaking the loop. Useful to prevent console barth during troubleshooting.
const int MAX_FAILURES   = 5;
const int MAX_DISTANCE   = 5;
const int MAX_EDITS_MADE = 5;

template <typename Func>
void RunLevenshteinTest(const char* function_name, Func applyEditFunc, const std::vector<std::string>& wordList, long long max_distance) {
    int failureCount = 0;
    for (int i = 0; i < LOOP; ++i) {
        std::string original = getUniformRandomString(wordList);
        int editCount = getRandomEditCount(original, MAX_EDITS_MADE);
        std::string modified = applyEditFunc(original, editCount);

        // We use `ALGORITHM_B` as the "expected" value, although this is somewhat arbitrary.
        ALGORITHM_B_SETUP();
        long long expected_distance = ALGORITHM_B_CALL(
                const_cast<char*>(original.c_str()),
                original.size(),
                const_cast<char*>(modified.c_str()),
                modified.size(),
                max_distance
        );
        ALGORITHM_B_TEARDOWN();

        // We record the test case
        TestCase testCase = {original, modified, expected_distance, function_name};

        // We consider `ALGORITHM_A` the algorithm under test, although this is somewhat arbitrary.
        algorithm_a_setup();
        long long result = algorithm_a_call(
                const_cast<char*>(original.c_str()),
                original.size(),
                const_cast<char*>(modified.c_str()),
                modified.size(),
                max_distance
        );
        algorithm_a_teardown();

        // Record failures
        if (result != testCase.expectedDistance) {
            PrintFailedTestCase(testCase, result);
            std::cout << "Max distance: " << max_distance << std::endl;
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


// Specific test for Transposition
TEST_F(LevenshteinTest, Transposition) {
    RunLevenshteinTest("Transposition", applyTransposition, wordList, MAX_DISTANCE);
}

// Specific test for Deletion
TEST_F(LevenshteinTest, Deletion) {
    RunLevenshteinTest("Deletion", applyDeletion, wordList, MAX_DISTANCE);
}

// Specific test for Insertion
TEST_F(LevenshteinTest, Insertion) {
    RunLevenshteinTest("Insertion", applyInsertion, wordList, MAX_DISTANCE);
}

// Specific test for Substitution
TEST_F(LevenshteinTest, Substitution) {
    RunLevenshteinTest("Substitution", applySubstitution, wordList, MAX_DISTANCE);
}

int main(int argc, char **argv) {
    initialize_metrics();
    std::cout << "BUFFER_SIZE: " << DAMLEV_BUFFER_SIZE << "\n";

    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "Testing " << algorithm_a_name << " against " << ALGORITHM_B_NAME << std::endl;
    int result = RUN_ALL_TESTS();

    print_metrics();
    return result;
}
