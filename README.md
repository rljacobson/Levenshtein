# Blazingly Fast Damerau–Levenshtein Edit Distance UDF for MySQL

This implementation is extremely fast and efficient, using both well-known and novel optimizations.<br><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_—R.J._
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_January 17, 2019_

Does the world really need another Levenshtein edit distance function for MySQL? YES. The most popular versions floating around on the internet are very slow and so poorly written as to be _dangerous_. Do not use them!

[FAQ](#faq)<br>
[Functions](#functions)<br>
[Usage](#usage)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[DAMLEV](#damlev)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[DAMLEVLIM](#damlevlim)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[DAMLEVP](#damlevp)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[DAMLEV2D](#damlevlimp)<br>
[Limitations](#limitations)<br>
[Requirements](#requirements)<br>
[Preparation for Use](#preparation-for-use)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[Acquiring prebuilt binaries](#acquiring-prebuilt-binaries)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[Building from source](#building-from-source)<br>
&nbsp;&nbsp;&nbsp;&nbsp;[Installing](#installing)<br>
[Warning](#warning)<br>
[Authors and License](#authors-and-license)

## FAQ
**Q:** How is Damerau-Levenshtein edit distance different from Levenshtein edit distance?

**A:** Levenshtein edit distance allows for insertions, deletions, and substitutions. Damerau-Levenshtein edit distance allows transposition in addition to the other three operations. Many "Levenshtein" functions are actually Damerau-Levenshtein functions. 

**Q:** What are the `*LIM` versions of the functions?

**A:** An optimization in which the algorithm will only perform the computation until the provided limit is reached. Then it returns the limit. This can be much more efficient if you only care about edit distance less than some known upper bound, a typical case with databases.

**Q:** What are the `*P` versions of the functions?

**A:** These functions return a normalized edit distance. The number returned is (edit distance)/(max string length), which is always a number in the interval `[0, 1]`. One interpretation of this is that of a percentage match. 

## Functions

| Function                                    | Description                                                                                                                                                                                                   |
|:--------------------------------------------|:--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `DAMLEV(STRING, STRING)`                    | Computes the Damerau-Levenshtein edit distance between two strings.                                                                                                                                           |
| `DAMLEVP(STRING, STRING)`                   | Computes a _normalized_ Damerau-Levenshtein **_similarity_** between two strings.                                                                                                                                 |
| `DAMLEVLIM(STRING, STRING, INT)`            | Computes the Damerau-Levenshtein edit distance between two strings up to a given max distance. Providing a max can significantly increase efficiency.                                                         |
| `DAMLEV2D(STRING, STRING)`                  | Computes the Levenshtein edit distance between two strings using a two row approach with optimization of vector length based on string lenght.                                                                |
| `DAMLEVCONST(STRING, CONSTANT STRING, INT)` | Computes the Damerau-Levenshtein edit distance between a string and a constant string up to a given max distance. Significant efficiency can result from the assumption that the second argument is constant. |

## Usage

#### DAMLEV

```sql
SELECT DAMLEV(String1, String2);
```

|  Argument | Meaning                                       |
|----------:|:----------------------------------------------|
| `String1` | A string                                      |
| `String2` | A string which will be compared to `String1`. |
| **Returns** | Either an integer equal to the edit distance between `String1` and `String2` or `PosInt`, whichever is smaller. |

**Example Usage:**

```sql
SELECT Name, DAMLEV(Name, "Vladimir Iosifovich Levenshtein") AS EditDist 
FROM CUSTOMERS WHERE DAMLEV(Name, "Vladimir Iosifovich Levenshtein") < 15;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 15 of "Vladimir Iosifovich Levenshtein".

#### DAMLEVLIM

```sql
SELECT DAMLEVLIM(String1, String2, PosInt);
```

|  Argument | Meaning                                       |
|----------:|:----------------------------------------------|
| `String1` | A string                                      |
| `String2` | A string which will be compared to `String1`. |
| **Returns** | An integer equal to the edit distance between `String1` and `String2` or `PosInt`, whichever is smaller. |

**Example Usage:**

```sql
SELECT Name, DAMLEVLIM(Name, "Vladimir Iosifovich Levenshtein", 6) AS EditDist 
FROM CUSTOMERS WHERE DAMLEVLIM(Name, "Vladimir Iosifovich Levenshtein", 6) < 6;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".

#### DAMLEVP

```sql
DAMLEVP(String1, String2);
```

|  Argument | Meaning                                                                                                                                                                                                                                                                                                          |
|----------:|:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `String1` | A string                                                                                                                                                                                                                                                                                                         |
| `String2` | A string which will be compared to `String1`.                                                                                                                                                                                                                                                                    |
| **Returns** | A floating point number in the range \[0, 1\] equal to the normalized similarity `similarity = 1.0 - edit_distance / max_string_length;` between `String1` and `String2`. This function is functionally equivalent to `1 - DAMLEV(String1, String2) / MAX(LENGTH(String1), LENGTH(String2))` but is much faster. |


#### Example Usage:

```sql
SELECT Name, DAMLEVP(Name, "Vladimir Iosifovich Levenshtein") AS EditDist 
FROM CUSTOMERS WHERE DAMLEVP(Name, "Vladimir Iosifovich Levenshtein") < 0.2;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 20% of "Vladimir Iosifovich Levenshtein".

#### DAMLEVCONST

```sql
DAMLEVCONST(String1, ConstString, PosInt);
```

|      Argument | Meaning                                                                 |
|--------------:|:------------------------------------------------------------------------|
|     `String1` | A string                                                                |
| `ConstString` | A constant string (string literal) which will be compared to `String1`. |
|      `PosInt` | A positive integer. If the distance between `String1` and `ConstString` is greater than `PosInt`, `DAMLEVCONST()` will stop its computation at `PosInt` and return `PosInt`. Make `PosInt` as small as you can to improve speed and efficiency. For example, if you put `WHERE DAMLEVCONST(...) < k` in a `WHERE`-clause, make `PosInt` be `k`. |
|   **Returns** | Either an integer equal to the edit distance between `String1` and `ConstString` or `PosInt`, whichever is smaller. |


#### Example Usage:

```sql
SELECT Name, DAMLEVCONST(Name, "Vladimir Iosifovich Levenshtein", 8) AS EditDist 
FROM CUSTOMERS WHERE DAMLEV(Name, "Vladimir Iosifovich Levenshtein", 8) < 8;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 8 of "Vladimir Iosifovich Levenshtein".


#### DAMLEV2D

This is a two row approach that is space optimized but does not calculate transpositions.
You can read about this approach here. 

https://takeuforward.org/data-structure/edit-distance-dp-33/
```sql
DAMLEV2D(String1, String2);
```

|    Argument | Meaning                                                                       |
|------------:|:------------------------------------------------------------------------------|
|   `String1` | A string                                                                      |
|   `String2` | A string which will be compared to `String1`.                                 |
| **Returns** | Either an integer equal to the edit distance between `String1` and `String2`. |


#### Example Usage:

```sql
SELECT Name, DAMLEV2D(Name, "Vladimir Iosifovich Levenshtein") AS EditDist 
FROM CUSTOMERS WHERE DAMLEV(Name, "Vladimir Iosifovich Levenshtein") < 8;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 8 of "Vladimir Iosifovich Levenshtein".

## Limitations

* This implementation assumes characters are represented as 8 bit `char`'s on your platform. If you are using UTF-8 codepoints above 255 (i.e. outside of UCS-2), this function will not
compute the correct edit distance between your strings.
* This function is case sensitive. If you need case insensitivity, you need to either compose this
function with `LOWER`/`TOLOWER`, or adapt the code.
* By default, `PosInt` has a default maximum of 512 for performance reasons. You can configure this maximum by changing
`DAMLEV_BUFFER_SIZE` in `CMakeLists.txt`. See the Configuration section below for more details.

Any one of these limitations would be a good for a contributor to solve. Make a pull
request!


## Requirements

* MySQL version 8.0 or greater. Or not. I'm not sure. That's just what I used.
* The headers for your version of MySQL.
* CMake. Who knows what minimum version is required, but it _should_ work on even very old versions. I used version 3.13.
* A C++ compiler released in the last decade. This code uses `constexpr`, which is a feature of C++11.

## Preparation for Use

### Acquiring prebuilt binaries

This is probably the easiest and fastest way to get going. Get pre-built binaries on [the Releases page](https://github.com/rljacobson/Levenshtein/releases). There are pre-built binaries for Linux, macOS, and Windows. Download the file and put it in your MySQL plugins directory. Then procede to the [Installing](#installing) section.

### Configuration

You can configure the following options within the `CMakeLists.txt` file.

| Option | Description | Values (Default) |
|:-----:|:-----|:-----|
| `BUFFER_SIZE` | Size of allocated buffer. This effectively limits the size of the strings you are able to compare. (See note below.) | unsigned long long number of bytes (`4096ull`, which is 4KB) |
| Insufficient Buffer Size Policy | Policy for handling strings that require a buffer size greater than the allocated buffer. (See note below.) | `TRUNCATE_ON_BUFFER_EXCEEDED` (default)<br>`RETURN_ZERO_ON_BUFFER_EXCEEDED`<br>`RETURN_NULL_ON_BUFFER_EXCEEDED` |
| Bad Max Policy | The behavior if the user provides a negative maximum edit distance. | `RETURN_ZERO_ON_BAD_MAX` (default)<br>`RETURN_NULL_ON_BAD_MAX` |

*Notes on buffer size.* 

For a single-row buffer, the size of the buffer required is just the size of the shortest string plus 1. The buffer is only used to compare the part of the strings *after* the common prefix and suffix are trimmed. Consequently, for any given set of strings being compared, you will only require a maximum of the size of the *second* largest string plus 1, as the largest string compared against itself will "trim" the entire string.

For a 2D matrix buffer, we require a buffer size of (shortest + 1)*(longest + 1).  You should therefore keep your strings smaller than $\sqrt{\text{buffer size}} - 1$.

There is a hard max set at ~16KB. This is the wrong tool for the job if you are using it for strings that large.
### Building from Docker

The Docker configuration is set up to persist the `build` directory. When you run the Docker container, the `.so` file will be generated in this directory. It's crucial to ensure that the chip architecture of your Docker environment matches your host machine to ensure compatibility with the `.so` file.

#### Steps to Build:

1. **Build and Start Docker Container**:
   Run the following command to build and start the Docker container. This command also triggers the building process of the `.so` file:

   ```bash
   docker-compose up --build
   ```

2. **Monitor the Output**:
   You may see some warning messages during the build process, typically related to type mismatches or other non-critical issues.

3. **Build Completion**:
   The build process is complete when you see an output similar to:

```bash
[+] Running 2/0
✔ Network blazingfastlevenshtein_default  Created                                                                                                                0.0s
✔ Container damlev_udf                    Created                                                                                                                0.0s
Attaching to damlev_udf
damlev_udf  | -- Using C++ standard: 17
damlev_udf  | -- Plugin dir: set(MYSQL_PLUGIN_DIR /usr/local/mysql/lib/plugin)
damlev_udf  | -- Include dir: -I/usr/include/mysql
damlev_udf  | -- C_FLAGS:  -Ofast -Wall -pedantic -Wextra -I/usr/include/mysql
damlev_udf  | -- Boost version: 1.65.1
damlev_udf  | -- Configuring done
damlev_udf  | -- Generating done
damlev_udf  | -- Build files have been written to: /code/build
damlev_udf  | Scanning dependencies of target oneoff
damlev_udf  | [  5%] Building CXX object CMakeFiles/oneoff.dir/tests/testoneoff.cpp.o
damlev_udf  | [ 11%] Building CXX object CMakeFiles/oneoff.dir/damlev.cpp.o
damlev_udf  | [ 16%] Linking CXX executable oneoff
damlev_udf  | [ 16%] Built target oneoff
damlev_udf  | Scanning dependencies of target unittest
damlev_udf  | [ 22%] Building CXX object CMakeFiles/unittest.dir/tests/unittests.cpp.o
damlev_udf  | [ 27%] Linking CXX executable unittest
damlev_udf  | [ 33%] Built target unittest
damlev_udf  | Scanning dependencies of target damlev
damlev_udf  | [ 38%] Building CXX object CMakeFiles/damlev.dir/tests/unittests.cpp.o
damlev_udf  | [ 44%] Linking CXX shared module libdamlev.so
damlev_udf  | [ 77%] Built target damlev
damlev_udf  | [100%] Built target benchmark
```

4. **Check the `build` Directory**:
   After the build is complete, the `.so` file can be found in the `build` directory on your host machine.

#### Notes:
- The `docker-compose up --build` command both builds the Docker image and starts the container as per the `docker-compose.yml` file.
- The build process executes inside the Docker container, but thanks to the configured volume mount, the output `.so` file is accessible on your host machine.
- Ensure compatibility between the Docker environment and your host machine, especially in terms of architecture (e.g., x86_64, ARM), for the `.so` file to function correctly.
- If you're not familiar with docker ask your favorite ChatBot 

---

This Markdown format maintains the structure and style you initially provided, offering clear and easy-to-follow instructions for building from Docker.
### Building from source

The usual CMake build process with `make`:

```bash
$ mkdir build
$ cd build
$ cmake .. 
$ make
$ make install
```

Alternatively, the usual CMake build process with `ninja`:

```bash
$ mkdir build
$ cd build
$ cmake .. -G Ninja 
$ ninja
$ ninja install
```

This will build the shared library `libdamlev.so` (`.dll` on Windows).

#### Troubleshooting the build

You can pass in `MYSQL_INCLUDE` and `MYSQL_PLUGIN_DIR` to tell CMake where to find `mysql.h` and where to install the plugin respectively. This is particularly helpful on Windows machines, which tend not to have `mysql_config` in the `PATH`:

```bash
$ cmake -DMYSQL_INCLUDE="C:\Program Files\MySQL\MySQL Server 8.0\include" -DMYSQL_PLUGIN_DIR="C:\Program Files\MySQL\MySQL Server 8.0\lib\plugin" .. 
```

1. The build script tries to find the required header files with `mysql_config --include`.
Otherwise, it takes a wild guess. Check to see if `mysql_config --plugindir` works on your command
line.
2. As in #1, the install script tries to find the plugins directory with
`mysql_config --plugindir`. See if that works on the command line.

### Installing

After building, install the shared library `libdamlev.so` to the plugins directory of your MySQL
installation:

```bash
$ sudo make install # or ninja install
$ mysql -u root
```

Enter your MySQL root user password to log in as root. Then follow the "usual" instructions for
installing a compiled UDF. Note that the names are case sensitive. Change out `.so` for `.dll` if you are on Windows.

```sql
CREATE FUNCTION damlev RETURNS INTEGER
  SONAME 'libdamlev.so';
CREATE FUNCTION damlevlim RETURNS INTEGER
  SONAME 'libdamlev.so';
CREATE FUNCTION damlevconst RETURNS INTEGER
  SONAME 'libdamlev.so';
CREATE FUNCTION damlevp RETURNS REAL
  SONAME 'libdamlev.so';
CREATE FUNCTION damlev2D RETURNS REAL
  SONAME 'libdamlev.so';
```

To uninstall:

```sql
DROP FUNCTION damlev;
DROP FUNCTION damlevlim;
DROP FUNCTION damlevp;
DROP FUNCTION damlev2D;
DROP FUNCTION damlevconst;
```

Then optionally remove the library file from the plugins directory:

```bash
$ rm /path/to/plugin/dir/libdamlev.so
```

You can ask MySQL for the plugin directory:

```bash
$ mysql_config --plugindir
/path/to/directory/
```

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
2. **Select Algorithm:** Ensure the `LEV_ARGS->arg_count` is set appropriately in `testharness.hpp` for the specific Damerau-Levenshtein implementation you wish to test (e.g., `damlevconst`, `damlev`, `damlevlimp`).
3. **Compile and Run:** Compile the script and execute it. The Damerau-Levenshtein distance between the specified strings will be displayed in the console.

#### Example Output

```
LEV_FUNCTION("string", "strlng") =  1
```

## Warning

__Warning:__ Do NOT use random code you found on the internet in a production
environment without auditing it carefully. Especially don't use anything called
`damlevlim256`. Yeah, I googled for Levenshtein UDFs just like you did. I found
some really heinous code. Horrible, very bad code that will give your children
lupus.

## Authors and License

Copyright (C) 2019 Robert Jacobson. Released under the MIT license.

Based on "Iosifovich", Copyright (C) 2019 Frederik Hertzum, which is
licensed under the MIT license: https://bitbucket.org/clearer/iosifovich.

The MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
