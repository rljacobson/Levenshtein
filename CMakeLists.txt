# root directory CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(Levenshtein)

# We use C++17 features.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall -pedantic -Wextra")

# Dynamic linking of shared libraries at runtime.
add_definitions("-DHAVE_DLOPEN")

# START CONFIGURATION

set(BUFFER_SIZE 4096ull)

# Select exactly one policy for handling negative max edit distance
add_compile_definitions(RETURN_ZERO_ON_BAD_MAX)
add_compile_definitions(TRUNCATE_ON_BUFFER_EXCEEDED)

# Use this file for a list of words for testing/benchmarking.
set(WORDS_FILE "/usr/share/dict/words")

# MySQL Include Directory
if(NOT DEFINED MYSQL_INCLUDE)
    set(MYSQL_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}")  # Set the path where mysql.h and udf_registration_types.h are located
endif()

# Include the MySQL directory
include_directories(${MYSQL_INCLUDE})

# Report configured values.
message(STATUS "Configured BUFFER_SIZE: ${BUFFER_SIZE}")
message(STATUS "MySQL Include dir: ${MYSQL_INCLUDE}")
message(STATUS "Configured WORDS_FILE for testing/benchmark: ${WORDS_FILE}")

# Add subdirectories
add_subdirectory(src)

option(BUILD_TESTS "Build the tests" OFF)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
