
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


### Building from source


The preferred CMake build process with `ninja`:

```bash
$ mkdir build
$ cd build
$ cmake .. -G Ninja 
$ ninja
$ ninja install
```

Alternatively, you can use the usual CMake build process with `make`:

```bash
$ mkdir build
$ cd build
$ cmake .. 
$ make
$ make install
```

This will build the shared library `libdamlev.so` (`.dll` on Windows, `.dylib` on macOS).

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
installing a compiled UDF. Note that the names are case sensitive. Change out `.so` for `.dll` if you are on Windows and `.dylib` if you are on macOS.

```sql
CREATE FUNCTION damlev RETURNS INTEGER
  SONAME 'libdamlev.so';
CREATE FUNCTION bounded_edit_dist_t RETURNS INTEGER
  SONAME 'libdamlev.so';
CREATE FUNCTION damlevconst RETURNS INTEGER
  SONAME 'libdamlev.so';
CREATE FUNCTION similarity_t RETURNS REAL
  SONAME 'libdamlev.so';
CREATE FUNCTION edit_dist_t_2d RETURNS REAL
  SONAME 'libdamlev.so';
```

To uninstall:

```sql
DROP FUNCTION damlev;
DROP FUNCTION bounded_edit_dist_t;
DROP FUNCTION similarity_t;
DROP FUNCTION edit_dist_t_2d;
DROP FUNCTION damlevconst;
```

Then optionally remove the library file from the plugins directory:

```bash
$ rm /path/to/plugin/dir/libdamlev.so
```

You can ask MySQL for the plugin directory:

```bash
$ mysql_config --plugindir
# /path/to/directory/
```
