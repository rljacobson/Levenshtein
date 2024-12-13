# tests/CMakeLists.txt

# Integrate Google Test

# Disable Google Test installation
set(INSTALL_GTEST OFF CACHE BOOL "Do not install GoogleTest headers")

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

# Define algorithm variables with default values
set(ALGORITHM "edit_dist_t_2d")

set(WORDS_PATH "/usr/share/dict/words" CACHE STRING "Path to the word list file.")
set(DAMLEV_BUFFER_SIZE 4096 CACHE STRING "Buffer size for the algorithm.")

# ===============================
# Executable Definitions
# ==============================

add_executable(unittest
        ${CMAKE_CURRENT_SOURCE_DIR}/comparetests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/edit_operations.cpp
        ../src/edit_dist.cpp
        ../src/bounded_edit_dist.cpp
        print_matrix.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/metrics.cpp
)
target_compile_definitions(unittest PRIVATE
        ALGORITHM_A=edit_dist
        ALGORITHM_A_COUNT=2
        ALGORITHM_B=bounded_edit_dist
        ALGORITHM_B_COUNT=3
        DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
        WORDS_PATH="${WORDS_PATH}"
        CAPTURE_METRICS
)
target_include_directories(unittest PRIVATE
#        ${PROJECT_SOURCE_DIR}/src/algorithms
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(unittest PRIVATE
        gtest_main
)


add_executable(compareoneoff
        ${CMAKE_CURRENT_SOURCE_DIR}/compareoneoff.cpp
        ../src/bounded_edit_dist.cpp
        ../src/postgres.cpp
        print_matrix.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/metrics.cpp
)
target_compile_definitions(compareoneoff PRIVATE
        ALGORITHM_A=bounded_edit_dist
        ALGORITHM_A_COUNT=3
        ALGORITHM_B=postgres
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
        ${DAMLEV_SOURCES}
        ../src/noop.cpp
        print_matrix.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/edit_operations.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/metrics.cpp
)
target_compile_definitions(benchmark PRIVATE
        DAMLEV_BUFFER_SIZE=${DAMLEV_BUFFER_SIZE}
        WORDS_PATH="${WORDS_PATH}"
#        CAPTURE_METRICS
        USE_GENERATED_WORDS # Comment this out to use words from WORDS_PATH
        EXCLUDE_PREPROCESSING_FROM_NOOP
)
target_compile_options(benchmark PRIVATE -O3 -ffast-math)
target_include_directories(benchmark PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(oneoff
        ${DAMLEV_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/testoneoff.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/metrics.cpp
)
target_compile_definitions(oneoff PRIVATE
        LEV_FUNCTION=min_edit_dist
        LEV_ALGORITHM_COUNT=3
        DAMLEV_BUFFER_SIZE=${DAMLEV_BUFFER_SIZE}
        WORDS_PATH="${WORDS_PATH}"
        CAPTURE_METRICS
)
target_include_directories(oneoff PRIVATE
#        ${PROJECT_SOURCE_DIR}/src/algorithms
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(oneoff PRIVATE
        gtest_main
)


# Discover and add GoogleTest. This is so we don't have to explicitly
# define all of our tests. We can just add the code and it magically
# works.
include(GoogleTest)
gtest_discover_tests(unittest)
