cmake_minimum_required(VERSION 3.10)

# Project name
project(Levenshtein)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall -pedantic -Wextra")

# Dynamic linking of shared libraries at runtime.
add_definitions("-DHAVE_DLOPEN")

# CONFIGURATION

# Modify the following to set the buffer size to something other than 4096
# characters. There is a hard max set at ~16KB. This is the wrong tool for
# the job if you are using it for strings that large.
set(BUFFER_SIZE 4096ull)

# Select the policy for handling negative max edit distance
add_compile_definitions(RETURN_ZERO_ON_BAD_MAX)
#add_compile_definitions(RETURN_NULL_ON_BAD_MAX)

# Select the policy for handling strings that require a buffer size greater than the allocated buffer.
add_compile_definitions(TRUNCATE_ON_BUFFER_EXCEEDED) # Truncates the strings to fit within the buffer.
#add_compile_definitions(RETURN_ZERO_ON_BUFFER_EXCEEDED)
#add_compile_definitions(RETURN_NULL_ON_BUFFER_EXCEEDED)


# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Integrate Google Test using FetchContent
include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1  # Use the desired GTest version
)

# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
