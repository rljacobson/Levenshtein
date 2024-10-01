# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.10)

# Project name
project(Levenshtein)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Print the C++ standard being used
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

# Enable export of compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall -pedantic -Wextra")

# Library specific definitions
add_definitions("-DHAVE_DLOPEN")

# Define library sources
set(DAMLEV_SOURCES
		damlev.cpp
		damlevp.cpp
		damlevlim.cpp
		# damlevlimp.cpp   ## TODO: Need to add this with a limit, say look for words with 80% similarity
		damlevconst.cpp
		damlevconstmin.cpp
		damlev2D.cpp
		noop.cpp
)

# Modify the buffer size if needed (default is 4096ull)
set(BUFFER_SIZE 4096ull)

# Add the damlev library
add_library(damlev MODULE ${DAMLEV_SOURCES} tests/unittests.cpp)
target_compile_definitions(damlev PRIVATE
		WORDS_PATH="/usr/share/dict/words"
		MYSQL_DYNAMIC_PLUGIN
		DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
)

### Library Installation ###

# Uncomment and set the MYSQL_PLUGIN_DIR if you know the plugin directory
# set(MYSQL_PLUGIN_DIR "/opt/homebrew/opt/mysql/lib/plugin/")

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MYSQL_INCLUDE}")
message(STATUS "Include dir: ${MYSQL_INCLUDE}")
message(STATUS "CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

install(TARGETS damlev LIBRARY DESTINATION ${MYSQL_PLUGIN_DIR})

### Testing and Benchmarking ###

# Use this file for a list of words
set(WORDS_FILE "/usr/share/dict/words")

# ===============================
# Define Algorithm Selection
# ===============================

# Define the list of available algorithms
set(AVAILABLE_ALGORITHMS damlev damlevconstmin damlevconst damlevlim damlevp noop)

# Define the ALGORITHM variable with a default value (Non-Cache Variable)
set(ALGORITHM "damlev")

# Validate the selected algorithm
if(NOT ALGORITHM IN_LIST AVAILABLE_ALGORITHMS)
	message(FATAL_ERROR "Invalid ALGORITHM specified: ${ALGORITHM}. Available options: ${AVAILABLE_ALGORITHMS}")
endif()

# Assign TEST_ALGORITHM and ARG_NUMBER based on ALGORITHM
if(ALGORITHM STREQUAL "damlev")
	set(TEST_ALGORITHM "damlev")
	set(ARG_NUMBER 2)
elseif(ALGORITHM STREQUAL "damlevconstmin")
	set(TEST_ALGORITHM "damlevconstmin")
	set(ARG_NUMBER 3)
elseif(ALGORITHM STREQUAL "damlevconst")
	set(TEST_ALGORITHM "damlevconst")
	set(ARG_NUMBER 3)
elseif(ALGORITHM STREQUAL "damlevlim")
	set(TEST_ALGORITHM "damlevlim")
	set(ARG_NUMBER 3)
elseif(ALGORITHM STREQUAL "damlevp")
	set(TEST_ALGORITHM "damlevp")
	set(ARG_NUMBER 3)
elseif(ALGORITHM STREQUAL "noop")
	set(TEST_ALGORITHM "noop")
	set(ARG_NUMBER 1)
endif()


# ===============================
# Print the selected algorithm and ARG_NUMBER for verification
# ===============================
message(STATUS "Selected Algorithm: ${TEST_ALGORITHM}")
message(STATUS "ARG_NUMBER: ${ARG_NUMBER}")

## Add Executable for One-off Testing
add_executable(oneoff common.h tests/testoneoff.cpp tests/testharness.hpp ${TEST_ALGORITHM}.cpp)
target_compile_definitions(oneoff PRIVATE
		LEV_FUNCTION=${TEST_ALGORITHM}
		LEV_ALGORITHM_COUNT=${ARG_NUMBER}
		DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
)

# Unit Tests and Benchmark uses Boost
find_package(Boost REQUIRED)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

## This is for unit testing for debugging purposes, makes errors of known and checks.
# TODO unittest does not currently work for damlevp, due to return type of a double conflicting.
add_executable(unittest common.h tests/unittests.cpp tests/testharness.hpp ${TEST_ALGORITHM}.cpp)
target_compile_definitions(unittest PRIVATE
		LEV_FUNCTION=${TEST_ALGORITHM}
		LEV_ALGORITHM_COUNT=${ARG_NUMBER}
		LEV_ALGORITHM_NAME=\"${TEST_ALGORITHM}\"
		DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
		WORDS_PATH="${WORDS_FILE}"
)

add_executable(benchmark common.h tests/testharness.hpp ${TEST_ALGORITHM}.cpp tests/benchmark.cpp)
target_compile_definitions(benchmark PRIVATE
		WORD_COUNT=235000ul
		BENCH_FUNCTION=${TEST_ALGORITHM}
		WORDS_PATH="${WORDS_FILE}"
		DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
)

# Optional: Specify C++ Standard (e.g., C++17)
set_target_properties(oneoff unittest benchmark PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED YES
)
