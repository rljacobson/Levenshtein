#include <iostream>
#include <string>
#include <vector>
#include "testharness.hpp"

int main() {
    struct TestCase {
        std::string a;
        std::string b;
        long long expectedDistance;
    };

    std::vector<TestCase> testCases = {
            {"test", "test", 0}, // Identical strings
            {"test", "tset", 1}, // Transposition
            {"test", "tes", 1},  // Deletion
            {"test", "teast", 1}, // Insertion
            {"test", "tezt", 1}, // Substitution
            {"", "", 0}, // Empty strings
            {"testa", "different", 8}, // Completely different
            {"test", "different", 7}, // same ending, but completely different

            {"Anguyyzarostrata", "Anguyzarostrata", 1}, // Original problematic case
            {"Anguyyzarotstrata", "Anguyzarostrata", 2}, // Additional transposition
            {"Anguyzarstrata", "Anguyzarostrata", 1}, // Deletion
            {"Anguyyzzarostrata", "Anguyzarostrata", 2}, // Insertion
            {"Anguyyzarostrata", "anguyzarostrata", 2}, // Mixed case sensitivity
            {"XAnguyyzarostrata", "YAnguyzarostrata", 2}, // Extended strings
            {"Anguyyzaros", "Anguyzaro", 2}, // Substring
            {"atartsorayzugnA", "atartsorazyugnA", 1}, // Reversed strings

            {"aMartha", "bMarbtahab", 4}, // Complex transposition and substitution
            {"", "", 0}, // Empty strings
            {"Anguilla", "Anguill", 1}, // Trailing character deletion
            {"Anguilla", "Anguillar", 1}, // Trailing character insertion
            {"Anguilla", "Anuiglla", 2}, // Internal transposition
            {"Anguilla", "Anguella", 1}, // Internal substitution
            {"Lysamata waurdemanni", "Lysamata wurdemanni", 1}, // One character difference
            {"Anguyyzarostrata", "Anguyzarostrata", 1}, // Original problematic string
            // Add more test cases as needed
    };


    long long maxDistance = 9; // Maximum edit distance

    LEV_SETUP();

    for (const auto& testCase : testCases) {
        long long result = LEV_CALL(const_cast<char*>(testCase.a.c_str()), testCase.a.size(),
                                    const_cast<char*>(testCase.b.c_str()), testCase.b.size(),
                                    maxDistance);

        bool testPassed = result == testCase.expectedDistance;
        std::cout <<  " (LD: " << result << ") - " << " (Expected: " << testCase.expectedDistance << ") - "
                  << (testPassed ? "PASS" : "FAIL") << std::endl;
    }

    LEV_TEARDOWN();
}
