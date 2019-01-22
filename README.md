# Damerau–Levenshtein Edit Distance UDF for MySQL 8.0+


This implementation is better than most others out there. It is extremely
fast and efficient.<br><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_—R.J._
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;_January 17, 2019_

## Usage

#### Syntax:

```sql
DAMLEV(String1, String2, PosInt);
```

| Argument  |  Meaning  |
|---:|:---|
|`String1` | A string constant or column. |
|`String2` | A string constant or column which will be compared to `String1`.
|`PosInt`  | A positive integer. If the distance between `String1` and `String2` is greater than `PosInt`, `DAMLEV()` will stop its computation at `PosInt` and return `PosInt`. Make `PosInt` as small as you can to improve speed and efficiency. For example, if you put `WHERE DAMLEV(...) < k` in a `WHERE`-clause, make `PosInt` be `k`. |

#### Example Usage:

```sql
SELECT Name, DAMLEV(Name, "Vladimir Iosifovich Levenshtein", 6) AS
    EditDist FROM CUSTOMERS WHERE EditDist < 6;
```

The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
where `Name` has edit distance within 6 of "Vladimir Iosifovich Levenshtein".


## Limitations:

* This implementation assumes characters are represented as `char`'s on your platform (probably 8
bits). If you are using UTF-8 codepoints above 255 (i.e. outside of UCS-2), this function will not
compute the correct edit distance between your strings.
* This function is case sensitive. If you need case insensitivity, you need to either compose this
function with `LOWER`/`TOLOWER`, or adapt the code.
* By default, `PosInt` has a default maximum of 500 for performance reasons. Removing the maximum
entirely is not supported at this time, but you can increase the default by defining
`DAMLEV_BUFFER_SIZE` to be a larger number:

```bash
export DAMLEV_BUFFER_SIZE=10000
```

Any one of these limitations would be a good for a first-time contributor to solve. Make a pull
request!


## Requirements

* MySQL version 8.0 or greater. Or not. I'm not sure. That's just what I used.
* The headers for your version of MySQL.
* CMake. Who knows what minimum version is required, but I used 3.13.
* A C++ compiler released in the last decade. This code uses features of C++17.

## Preparation for Use

### Building

The usual CMake build process with `make`:

```bash
$ mkdir build
$ cd build
$ cmake .. && make
$ make install
```

Alternatively, the usual CMake build process with `ninja`:

```bash
$ mkdir build
$ cd build
$ cmake .. -G Ninja && ninja
$ ninja install
```

This will build the shared library `libdamlev.so` (`.dll` on Windows).

#### Troubleshooting the build

1. The build script tries to find the required header files with `mysql_config --include`.
Otherwise, it takes a wild guess. Check to see if `mysql_config --plugindir` works on your command
line.
2. As in #1, the install script tries to find the plugins directory with
`mysql_config --plugindir`. See if that works on the command line.

### Installing

After building, install the shared library `damlev_udf.so` to the plugins directory of your MySQL
installation:

```bash
$ sudo make install # or ninja install
$ mysql -u root
```

Enter your MySQL root user password to log in as root. Then follow the "usual" instructions for
installing a compiled UDF:

```sql
CREATE FUNCTION damlev RETURNS INTEGER SONAME 'libdamlev.so';
```

To uninstall:

```sql
DROP FUNCTION damlev;
```

Then optionally remove the library file from the plugins directory:

```bash
$ rm /path/to/plugin/dir/damlev_udf.so
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
