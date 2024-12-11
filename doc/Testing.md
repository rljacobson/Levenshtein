## Testing

### Unittest.cpp

#### Overview

The unit testing suite focuses on evaluating several key types of string manipulations, each contributing to the robust testing of our algorithms, especially in calculating the Damerau-Levenshtein distance. These manipulations simulate real-world scenarios where strings may undergo various changes. The primary types of manipulations included in the test suite are:

#### 1. Transposition

- **Description:** This manipulation involves swapping adjacent characters in a string.
- **Purpose:** Tests the algorithm's ability to handle cases where characters are out of order but still present, a common occurrence in typing errors.

#### 2. Deletion

- **Description:** Involves removing one or more characters from the string.
- **Purpose:** Validates the algorithm’s performance when characters are missing, reflecting scenarios like typos or truncated text.

#### 3. Insertion

- **Description:** This manipulation adds one or more new characters to the string.
- **Purpose:** Assesses how well the algorithm copes with extraneous characters, which could represent data entry errors or unintended insertions.

#### 4. Substitution

- **Description:** Involves replacing one or more characters in the string with different characters.
- **Purpose:** Tests the algorithm’s accuracy in situations where characters are mistakenly replaced, a typical issue in misspelled words.

#### 5. Common Prefix/Suffix Addition

- **Description:** Adds a common set of characters either at the beginning (prefix) or at the end (suffix) of the string.
- **Purpose:** Ensures the algorithm remains effective when strings share similar starting or ending segments, a frequent case in related or derived words.

#### Key Features of the Testing Suite

- **Source of Test Words:** The suite sources test words from a predefined file (`tests/taxanames`), with a secondary option of using a system dictionary (`/usr/share/dict/words`). You will likely not have (`tests/taxanames`), feel free to build file with a million rows of long strings for testing. This strategy offers a rich and diverse pool of test strings.
- **Randomization Mechanism:** Utilizing a standard library random device paired with a Mersenne Twister engine (`std::mt19937`), the suite efficiently generates random numbers to facilitate the creation of various test conditions.

#### Structure and Execution of Test Cases

- **TestCase Structure:** Each test case, defined in the `TestCase` structure, includes two strings and an expected Damerau-Levenshtein distance, accompanied by a descriptive label of the function being tested.
- **Diverse String Operations:** The suite employs an array of string manipulations, including transposition, deletion, insertion, and substitution, applied randomly to simulate real-world string alterations.
- **Testing Common Prefixes/Suffixes:** The test strings are systematically appended with common prefixes and suffixes, extending up to 10 characters, to evaluate the algorithm's performance with shared string segments.
- **Extensive Iterative Testing:** Executing a substantial number of iterations (as indicated by `LOOP`), the suite ensures broad coverage of potential string scenarios.

#### Error Management and Reporting

- **Automated Execution and Validation:** The testing process is fully automated, comparing the calculated Damerau-Levenshtein distances against pre-established benchmarks.
- **Efficient Error Handling:** The suite is designed to cease execution if more than 25 errors are detected, returning those error and a code of 99. This approach prevents extensive logging from a single pervasive issue, enhancing the efficiency of error identification and resolution.
- **Successful Test Message:**  `ALL PASSED FOR 100000 STRINGS`

### One Off Test `testoneoff.cpp`

#### Overview

The one-off test script is a specialized utility designed to manually test the calculation of the Damerau-Levenshtein distance between two specific strings. This tool is particularly useful for quickly debugging the functionality of different variations of the Damerau-Levenshtein algorithm.

#### Features

- **Customizable String Inputs:** Users can specify any two strings for comparison directly within the code. The script is currently set up to compare `"string"` and `"strlng"`.
- **Flexible Algorithm Testing:** The script is adaptable to test different implementations of the Damerau-Levenshtein distance by adjusting the `LEV_ARGS->arg_count` and corresponding function in `testharness.hpp`.
- **Immediate Results:** It provides an instant calculation of the Damerau-Levenshtein distance, outputting the result to the console.

#### Usage

1. **Setup Strings:** Modify the `string_a` and `string_b` variables in the script to the strings you want to compare.
2. **Select Algorithm:** Ensure the `LEV_ARGS->arg_count` is set appropriately in `testharness.hpp` for the specific Damerau-Levenshtein implementation you wish to test (e.g., `damlevconst`, `damlev`, `bounded_edit_dist_tp`).
3. **Compile and Run:** Compile the script and execute it. The Damerau-Levenshtein distance between the specified strings will be displayed in the console.

#### Example Output

```
LEV_FUNCTION("string", "strlng") =  1
```

## Warning

__Warning:__ Do NOT use random code you found on the internet in a production
environment without auditing it carefully. Especially don't use anything called
`bounded_edit_dist_t256`. Yeah, I googled for Levenshtein UDFs just like you did. I found
some really heinous code. Horrible, very bad code that will give your children
lupus.
