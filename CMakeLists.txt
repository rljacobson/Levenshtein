cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

project(Levenshtein)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall -pedantic -Wextra")

# Library specific.
add_definitions("-DHAVE_DLOPEN")

set(DAMLEV_SOURCES
        damlev.cpp
        damlevp.cpp
        damlevlim.cpp
#       damlevlimp.cpp   ## TODO Need to add this with a limit, say look for words with 80% similar..
		damlevconst.cpp
		damlevconstmin.cpp
		damlev2D.cpp
		noop.cpp
)


# Modify the following to set the buffer size to something other than 4096
# characters. There is a hard max set at ~16KB. This is the wrong tool for
# the job if you are using it for strings that large.
set(BUFFER_SIZE 4096ull)


add_library(damlev MODULE ${DAMLEV_SOURCES} tests/unittests.cpp)
target_compile_definitions(damlev PRIVATE WORDS_PATH="/usr/share/dict/words")
target_compile_definitions(damlev PRIVATE MYSQL_DYNAMIC_PLUGIN)
target_compile_definitions(damlev PRIVATE DAMLEV_BUFFER_SIZE=${BUFFER_SIZE})


### Library Installation ###

# If you know the plugin directory of your MySQL installation, you can uncomment
# the following line and set it here.

set(MYSQL_PLUGIN_DIR "set(MYSQL_PLUGIN_DIR /opt/homebrew/opt/mysql/lib/plugin/)")

if(NOT DEFINED MYSQL_PLUGIN_DIR)
    if(INSTALL_PLUGINDIR)
        set(MYSQL_PLUGIN_DIR ${INSTALL_PLUGINDIR})
    else()
		# Note: `mysql_config` must be in the PATH for this to work.
        execute_process(COMMAND mysql_config --plugindir
                OUTPUT_VARIABLE MYSQL_PLUGIN_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
endif()
message(STATUS "Plugin dir: ${MYSQL_PLUGIN_DIR}")

execute_process(COMMAND mysql_config --include
        OUTPUT_VARIABLE MYSQL_INCLUDE
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if(WIN64)
	# This is not at all robust. We need a better solution.
	if(NOT DEFINED MYSQL_INCLUDE)
		set(MYSQL_INCLUDE "C:\\Program Files\\MySQL\\MySQL Server 8.0\\include")
	endif()
	if(NOT DEFINED MYSQL_PLUGIN_DIR)
		set(MYSQL_PLUGIN_DIR "C:\\Program Files\\MySQL\\MySQL Server 8.0\\lib\\plugin")
	endif()
endif()
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${MYSQL_INCLUDE}")
message(STATUS "Include dir: ${MYSQL_INCLUDE}")
message(STATUS "C_FLAGS: ${CMAKE_CXX_FLAGS}")

install(TARGETS damlev LIBRARY DESTINATION ${MYSQL_PLUGIN_DIR})

### Testing and Benchmarking ###

# Use this file for a list of words
set(WORDS_FILE "/usr/share/dict/words")

# Select which algorithm to test by uncommenting exactly one of the following.
#set(TEST_ALGORITHM "damlevconstmin")
set(TEST_ALGORITHM "damlevconst")
#set(TEST_ALGORITHM "damlevlim")
#set(TEST_ALGORITHM "damlevp")
#set(TEST_ALGORITHM "noop")

## This is for one-off testing for debugging purposes.
#### you will need to adjust arg number for those with limits
add_executable(oneoff common.h tests/testoneoff.cpp tests/testharness.hpp ${TEST_ALGORITHM}.cpp)
target_compile_definitions(oneoff PRIVATE LEV_FUNCTION=${TEST_ALGORITHM})
target_compile_definitions(oneoff PRIVATE DAMLEV_BUFFER_SIZE=${BUFFER_SIZE})

# Unit Tests and Benchmark uses Boost
find_package(Boost REQUIRED)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

## This is for unit testing for debugging purposes, makes errors of known and checks.
#TODO unittest does not currently work for damlevp, due to return type of a double conflicting.
add_executable(unittest common.h tests/unittests.cpp tests/testharness.hpp ${TEST_ALGORITHM}.cpp)
target_compile_definitions(unittest PRIVATE LEV_FUNCTION=${TEST_ALGORITHM})
target_compile_definitions(unittest PRIVATE DAMLEV_BUFFER_SIZE=${BUFFER_SIZE})
target_compile_definitions(unittest PRIVATE WORDS_PATH="${WORDS_FILE}")

add_executable(benchmark common.h tests/testharness.hpp damlev.cpp ${TEST_ALGORITHM}.cpp tests/benchmark.cpp)
target_compile_definitions(benchmark PRIVATE WORD_COUNT=235000ul)
target_compile_definitions(benchmark PRIVATE BENCH_FUNCTION=${TEST_ALGORITHM})
target_compile_definitions(benchmark PRIVATE WORDS_PATH="${WORDS_FILE}")
target_compile_definitions(benchmark PRIVATE DAMLEV_BUFFER_SIZE=${BUFFER_SIZE})
