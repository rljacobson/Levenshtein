/*!

Utility functions to apply random edits to a string in place.

*/

#pragma once

#include <string>
#include <vector>

/**
Applies transposition edits to a string.

Transposition involves swapping adjacent characters in the string.
The number of transpositions is specified by the editCount parameter.
If the string is less than 2 characters long, no changes are made.

@param str The input string to be edited.
@param editCount The number of transpositions to apply.
@return The modified string with transpositions applied.
*/
std::string applyTransposition(std::string str, int editCount);

/**
Applies deletion edits to a string.

Deletion involves removing characters from the string at random positions.
The number of deletions is specified by the editCount parameter.
If the string is empty, no changes are made.

@param str The input string to be edited.
@param editCount The number of deletions to apply.
@return The modified string with deletions applied.
*/
std::string applyDeletion(std::string str, int editCount);

/**
Applies insertion edits to a string.

Insertion involves adding random characters at random positions in the string.
The number of insertions is specified by the editCount parameter.

@param str The input string to be edited.
@param editCount The number of insertions to apply.
@return The modified string with insertions applied.
*/
std::string applyInsertion(const std::string& str, int editCount);

/**
Applies substitution edits to a string.

Substitution involves replacing characters in the string with random characters.
The number of substitutions is specified by the editCount parameter.
If the string is empty, no changes are made.

@param str The input string to be edited.
@param editCount The number of substitutions to apply.
@return The modified string with substitutions applied.
*/
std::string applySubstitution(const std::string& str, int editCount);

/**
Retrieves a random string from a given list of words.

This function selects a random string from the provided wordList vector.
If the list is empty, an empty string is returned.

@param wordList A vector of strings from which to select a random string.
@return A random string selected from the wordList, or an empty string if the list is empty.
*/
std::string getUniformRandomString(const std::vector<std::string>& wordList);

/// Only return 0 for empty strings. Otherwise, return at least 1.
int getRandomEditCount(const std::string& str, int maximum_edits);

/// Generate a random letter based on letter frequencies in written English.
char getRandomLetter();

/// Generate a random string of the given length based on letter
/// frequencies in written English.
std::string generateRandomString(int length);
