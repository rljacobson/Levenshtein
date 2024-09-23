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
		damlev2D.cpp
		noop.cpp
   )


add_library(damlev MODULE ${DAMLEV_SOURCES} tests/unittests.cpp)
target_compile_definitions(damlev PRIVATE WORDS_PATH="/usr/share/dict/words")
target_compile_definitions(damlev PRIVATE MYSQL_DYNAMIC_PLUGIN)
# Uncomment the following to set the buffer size to something other than 512
# characters. There is a hard max set at ~16KB. This is the wrong tool for
# the job if you are using it for strings that large.
#   target_compile_definitions(damlev PRIVATE DAMLEV_BUFFER_SIZE=4096ull)

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

## This is for one-off testing for debugging purposes.
#### you will need to adjust arg number for those with limits
add_executable(oneoff common.h tests/testoneoff.cpp tests/testharness.hpp damlevconst.cpp)
target_compile_definitions(oneoff PRIVATE LEV_FUNCTION=damlevconst)

## This is for unit testing for debugging purposes, makes errors of known and checks.
#TODO unittest does not currently work for damlevp, due to return type of a double conflicting.
add_executable(unittest common.h tests/unittests.cpp tests/testharness.hpp damlevconst.cpp)
target_compile_definitions(unittest PRIVATE LEV_FUNCTION=damlevconst)


# Benchmark
find_package(Boost REQUIRED)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(benchmark common.h tests/testharness.hpp damlev.cpp noop.cpp tests/benchmark.cpp)
target_compile_definitions(benchmark PRIVATE WORD_COUNT=235000ul)
target_compile_definitions(benchmark PRIVATE BENCH_FUNCTION=damlevconst)
target_compile_definitions(benchmark PRIVATE WORDS_PATH="/usr/share/dict/words")
