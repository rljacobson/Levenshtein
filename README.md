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
&nbsp;&nbsp;&nbsp;&nbsp;[DAMLEVLIMP](#damlevlimp)<br>
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

| Function                                      | Description  |
|:--------------------------------------------|:------------|
| `DAMLEV(STRING, STRING)`                      | Computes the Damerau-Levenshtein edit distance between two strings. |
| `DAMLEVP(STRING, STRING)`                     | Computes a _normalized_ Damerau-Levenshtein edit distance between two strings. |
| `DAMLEVLIM(STRING, STRING, INT)`              | Computes the Damerau-Levenshtein edit distance between two strings up to a given max distance. Providing a max can significantly increase efficiency. |
| `DAMLEVCONST(STRING, CONSTANT STRING, FLOAT)` | Computes the Damerau-Levenshtein edit distance between a string and a constant string up to a given max distance. Significant efficiency can result from the assumption that the second argument is constant. |

## Usage

#### DAMLEV

```sql
SELECT DAMLEV(String1, String2);
```

|  Argument | Meaning                                       |
|----------:|:----------------------------------------------|
| `String1` | A string                                      |
| `String2` | A string which will be compared to `String1`. |
|    `PosInt` | A positive integer. If the distance between `String1` and `String2` is greater than `PosInt`, `DAMLEVLIM()` will stop its computation at `PosInt` and return `PosInt`. Make `PosInt` as small as you can to improve speed and efficiency. For example, if you put `WHERE DAMLEVLIM(...) < k` in a `WHERE`-clause, make `PosInt` be `k`. |
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

|  Argument | Meaning                                       |
|----------:|:----------------------------------------------|
| `String1` | A string                                      |
| `String2` | A string which will be compared to `String1`. |
| **Returns** | A floating point number in the range \[0, 1\] equal to the normalized edit distance between `String1` and `String2`. This function is functionally equivalent to `DAMLEV(String1, String2)/MAX(LENGTH(String1), LENGTH(String2))` but is much faster. |


#### Example Usage:

```sql
SELECT Name, DAMLEVP(Name, "Vladimir Iosifovich Levenshtein") AS EditDist 
FROM CUSTOMERS WHERE DAMLEVP(Name, "Vladimir Iosifovich Levenshtein") < 0.2;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 20% of "Vladimir Iosifovich Levenshtein".

#### DAMLEVCONST

```sql
DAMLEVLIMP(String1, ConstString, PosInt);
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

## Limitations

* This implementation assumes characters are represented as 8 bit `char`'s on your platform. If you are using UTF-8 codepoints above 255 (i.e. outside of UCS-2), this function will not
compute the correct edit distance between your strings.
* This function is case sensitive. If you need case insensitivity, you need to either compose this
function with `LOWER`/`TOLOWER`, or adapt the code.
* There are a few edge cases that that give an erroneous damlev2D, 1 less than the actual distance
* By default, `PosInt` has a default maximum of 512 for performance reasons. Removing the maximum
entirely is not supported at this time, but you can increase the default by defining
`DAMLEV_BUFFER_SIZE` to be a larger number prior to compilation:

```bash
$ export DAMLEV_BUFFER_SIZE=10000
```

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
CREATE FUNCTION damlevlimp RETURNS REAL
  SONAME 'libdamlev.so';
```

To uninstall:

```sql
DROP FUNCTION damlev;
DROP FUNCTION damlevlim;
DROP FUNCTION damlevp;
DROP FUNCTION damlevlimp;
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
