/*!

Utility functions to apply random edits to a string in place.

*/


#include <random>

#include "edit_operations.hpp"

// Random Number Generator
std::random_device rd;
std::mt19937 gen(rd());

// Cumulative frequencies of (lowercase) letters in written English.

const double cumulativeFrequencies[26] = {
    0.08167, 0.09659, 0.12441, 0.16694,
    0.29396, 0.31624, 0.33639, 0.39733,
    0.46699, 0.46852, 0.47624, 0.51649,
    0.54055, 0.60804, 0.68311, 0.70240,
    0.70335, 0.76322, 0.82649, 0.91705,
    0.94502, 0.95480, 0.97840, 0.97990,
    0.98064, 1.00000
};

// A few random utility functions.

int getRandomInt(int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

char getUniformRandomChar() {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    return chars[dis(gen)];
}

/// Only return 0 for empty strings. Otherwise, return at least 1.
int getRandomEditCount(const std::string& str, int maximum_edits = 5) {
    if (str.empty()) return 0;
    return getRandomInt(1, std::min(static_cast<int>(str.length()), maximum_edits)); // Limiting to a max of 5 for practicality
}

/**
Applies transposition edits to a string in place.

Transposition involves swapping adjacent characters in the string.
The number of transpositions is specified by the editCount parameter.
If the string is less than 2 characters long, no changes are made.

@param str The input string to be edited.
@param editCount The number of transpositions to apply.
@return The modified string with transpositions applied.
*/
std::string applyTransposition(std::string str, int editCount) {
    for (int i = 0; i < editCount; ++i) {
        if (str.length() < 2) break;
        int pos = getRandomInt(0, str.length() - 2);
        std::swap(str[pos], str[pos + 1]);
    }
    return str;
}

/**
Applies deletion edits to a string in place.

Deletion involves removing characters from the string at random positions.
The number of deletions is specified by the editCount parameter.
If the string is empty, no changes are made.

@param str The input string to be edited.
@param editCount The number of deletions to apply.
@return The modified string with deletions applied.
*/
std::string applyDeletion(std::string str, int editCount) {
    for (int i = 0; i < editCount; ++i) {
        if (!str.empty()) {
            int pos = getRandomInt(0, str.length() - 1);
            str.erase(pos, 1);
        }
    }
    return str;
}

/**
Applies insertion edits to a string in place.

Insertion involves adding random characters at random positions in the string.
The number of insertions is specified by the editCount parameter.

@param str The input string to be edited.
@param editCount The number of insertions to apply.
@return The modified string with insertions applied.
*/
std::string applyInsertion(const std::string& str, int editCount) {
    std::string result = str;
    for (int i = 0; i < editCount; ++i) {
        int pos = getRandomInt(0, result.length());
        char ch = getUniformRandomChar();
        result.insert(pos, 1, ch);
    }
    return result;
}

/**
Applies substitution edits to a string in place.

Substitution involves replacing characters in the string with random characters.
The number of substitutions is specified by the editCount parameter.
If the string is empty, no changes are made.

@param str The input string to be edited.
@param editCount The number of substitutions to apply.
@return The modified string with substitutions applied.
*/
std::string applySubstitution(const std::string& str, int editCount) {
    std::string result = str;
    for (int i = 0; i < editCount; ++i) {
        if (!result.empty()) {
            int pos = getRandomInt(0, result.length() - 1);
            result[pos] = getUniformRandomChar();
        }
    }
    return result;
}

/**
Retrieves a random string from a given list of words.

This function selects a random string from the provided wordList vector.
If the list is empty, an empty string is returned.

@param wordList A vector of strings from which to select a random string.
@return A random string selected from the wordList, or an empty string if the list is empty.
*/
std::string getUniformRandomString(const std::vector<std::string>& wordList) {
    if (wordList.empty()) return "";
    int index = getRandomInt(0, wordList.size() - 1);
    return wordList[index];
}

/// Generate a random letter based on letter frequencies in written English.
char getRandomLetter() {
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    double randomValue = distribution(gen);

    for (std::size_t i = 0; i < 26; ++i) {
        if (randomValue < cumulativeFrequencies[i]) {
            return static_cast<char>('a' + i);
        }
    }

    return 'z'; // Fallback, should not reach here
}

/// Generate a random string of the given length based on letter
/// frequencies in written English.
std::string generateRandomString(int length) {
    std::string result;
    result.reserve(length);

    for (int i = 0; i < length; ++i) {
        result += getRandomLetter();
    }
    return result;
}
