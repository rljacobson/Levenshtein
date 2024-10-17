/*!

Utility functions to apply random edits to a string in place.

*/


#include <random>

#include "edit_operations.hpp"

// Random Number Generator
std::random_device rd;
std::mt19937 gen(rd());

// A few random utility functions.

int getRandomInt(int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

char getRandomChar() {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    return chars[dis(gen)];
}

/// Only return 0 for empty strings. Otherwise, return at least 1.
int getRandomEditCount(const std::string& str) {
    if (str.empty()) return 0;
    return getRandomInt(1, std::min(static_cast<int>(str.length()), 5)); // Limiting to a max of 5 for practicality
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
        char ch = getRandomChar();
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
            result[pos] = getRandomChar();
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
std::string getRandomString(const std::vector<std::string>& wordList) {
    if (wordList.empty()) return "";
    int index = getRandomInt(0, wordList.size() - 1);
    return wordList[index];
}
