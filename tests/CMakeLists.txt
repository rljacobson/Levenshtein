# Enable testing
enable_testing()

# Find required packages
find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# Integrate Google Test using FetchContent
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1  # Specify the desired GTest version
)

# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ===============================
# Algorithm Selection Section
# ===============================
# Define available algorithms and their argument counts
set(AVAILABLE_ALGORITHMS "damlev;damlevconstmin;damlevconst;damlevlim;damlevp;noop")
set(ALGORITHM_ARGS "2;3;3;3;3;1")

# Define algorithm variables with default values
set(ALGORITHM "damlevconstmin")
set(WORDS_PATH "/usr/share/dict/words" CACHE STRING "Path to the word list file.")
set(DAMLEV_BUFFER_SIZE 4096ull CACHE STRING "Buffer size for the algorithm.")

# Validate selected algorithm and extract its argument count
list(FIND AVAILABLE_ALGORITHMS ${ALGORITHM} ALGORITHM_INDEX)
if(ALGORITHM_INDEX EQUAL -1)
    message(FATAL_ERROR "Invalid ALGORITHM specified: ${ALGORITHM}. Available options: ${AVAILABLE_ALGORITHMS}")
endif()
list(GET AVAILABLE_ALGORITHMS ${ALGORITHM_INDEX} TEST_ALGORITHM)
list(GET ALGORITHM_ARGS ${ALGORITHM_INDEX} ARG_NUMBER)

# ===============================
# Functions for Common Operations
# ===============================
# Function to set up executables with common properties
function(setup_executable target source_file)
    add_executable(${target} ${source_file})
    target_compile_definitions(${target} PRIVATE
            LEV_FUNCTION=${TEST_ALGORITHM}
            LEV_ALGORITHM_COUNT=${ARG_NUMBER}
            LEV_ALGORITHM_NAME=\"${TEST_ALGORITHM}\"
            DAMLEV_BUFFER_SIZE=${DAMLEV_BUFFER_SIZE}
            WORDS_PATH=\"${WORDS_PATH}\"
            TEST_ALGORITHM=\"${TEST_ALGORITHM}\"
    )
    target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${PROJECT_SOURCE_DIR}/include
    )
    target_link_libraries(${target} PRIVATE
            damlev
            gtest_main
    )
endfunction()

# ===============================
# Executable Definitions
# ===============================
# Define executables for one-off testing, unit testing, and benchmarking
setup_executable(oneoff ${CMAKE_CURRENT_SOURCE_DIR}/testoneoff.cpp)
setup_executable(unittest ${CMAKE_CURRENT_SOURCE_DIR}/unittests.cpp)
setup_executable(benchmark ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cpp)

# Adjust specific compile definitions for each executable
target_compile_definitions(benchmark PRIVATE
        WORD_COUNT=235000ul
        BENCH_FUNCTION=${TEST_ALGORITHM}
)

# ===============================
# Testing and Benchmarking
# ===============================
# Discover and add GoogleTest
include(GoogleTest)
gtest_discover_tests(unittest)

# Set C++ Standard for all executables
set_target_properties(oneoff unittest benchmark PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
)
