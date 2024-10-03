# Root CMakeLists.txt

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

# Set global compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall -pedantic -Wextra")

# Library specific definitions
add_definitions("-DHAVE_DLOPEN")

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
