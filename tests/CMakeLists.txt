# Enable testing
enable_testing()

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

# Find required packages
find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# ===============================
# Algorithm Selection Section
# ===============================
# Define available algorithms and their argument counts
set(AVAILABLE_ALGORITHMS "lev;levlim;levlimopt;damlev;damlev2D;damlevlim;damlevmin;damlevminp;damlevp;noop")
set(ALGORITHM_ARGS "2;3;3;2;2;3;3;3;3;3")

# Define algorithm variables with default values
set(ALGORITHM "damlev2D")

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
            DAMLEV_BUFFER_SIZE=${DAMLEV_BUFFER_SIZE}
            WORDS_PATH="${WORDS_PATH}"
            TEST_ALGORITHM="${TEST_ALGORITHM}"
            CAPTURE_METRICS
    )
    target_include_directories(${target} PRIVATE
            ${PROJECT_SOURCE_DIR}/src/algorithms
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(${target} PRIVATE
            damlev
            gtest_main
    )
endfunction()

# ===============================
# Executable Definitions
# ==============================

add_executable(unittest
        ${CMAKE_CURRENT_SOURCE_DIR}/comparetests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/edit_operations.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/lev.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlev.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/printmatrix.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/metrics.cpp
)
target_compile_definitions(unittest PRIVATE
        ALGORITHM_A=lev
        ALGORITHM_A_COUNT=2
        ALGORITHM_B=damlev
        ALGORITHM_B_COUNT=2
        DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
        WORDS_PATH="${WORDS_PATH}"
        CAPTURE_METRICS
)
target_include_directories(unittest PRIVATE
        ${PROJECT_SOURCE_DIR}/src/algorithms
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(unittest PRIVATE
#        damlev
        gtest_main
)


add_executable(compareoneoff
        ${CMAKE_CURRENT_SOURCE_DIR}/compareoneoff.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlevlim.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlevmin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/printmatrix.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/metrics.cpp
)
target_compile_definitions(compareoneoff PRIVATE
        ALGORITHM_A=damlevlim
        ALGORITHM_A_COUNT=3
        ALGORITHM_B=damlevmin
        ALGORITHM_B_COUNT=3
        DAMLEV_BUFFER_SIZE=${DAMLEV_BUFFER_SIZE}
        CAPTURE_METRICS
        PRINT_DEBUG
)
target_include_directories(compareoneoff PRIVATE
        ${PROJECT_SOURCE_DIR}/src/algorithms
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(benchmark
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlev.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlev2D.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlevlim.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlevmin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlevminp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/damlevp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/lev.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/levlim.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/levlimopt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/noop.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/printmatrix.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/edit_operations.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/metrics.cpp
)
target_compile_definitions(benchmark PRIVATE
#        ALGORITHM_A=damlevlim
#        ALGORITHM_A_COUNT=3
#        ALGORITHM_B=damlev1D
#        ALGORITHM_B_COUNT=3
        DAMLEV_BUFFER_SIZE=${DAMLEV_BUFFER_SIZE}
        WORDS_PATH="${WORDS_PATH}"
        CAPTURE_METRICS
#        PRINT_DEBUG
)
target_compile_options(benchmark PRIVATE -O3 -ffast-math)
target_include_directories(benchmark PRIVATE
        ${PROJECT_SOURCE_DIR}/src/algorithms
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define executables for one-off testing, unit testing, and benchmarking
setup_executable(oneoff ${CMAKE_CURRENT_SOURCE_DIR}/testoneoff.cpp)
#setup_executable(benchmark ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cpp)

# Adjust specific compile definitions for each executable
target_compile_definitions(benchmark PRIVATE
        WORD_COUNT=235000ul
        BENCH_FUNCTION="${TEST_ALGORITHM}"
)

# ===============================
# Testing and Benchmarking
# ===============================
# Discover and add GoogleTest
include(GoogleTest)
gtest_discover_tests(unittest)
