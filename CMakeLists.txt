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
        damlevlimp.cpp
		damlevconst.cpp
		noop.cpp
   )


add_library(damlev MODULE ${DAMLEV_SOURCES})
target_compile_definitions(damlev PRIVATE WORDS_PATH="/usr/share/dict/words")
target_compile_definitions(damlev PRIVATE MYSQL_DYNAMIC_PLUGIN)
# Uncomment the following to set the buffer size to something other than 512
# characters. There is a hard max set at ~16KB. This is the wrong tool for
# the job if you are using it for strings that large.
#   target_compile_definitions(damlev PRIVATE DAMLEV_BUFFER_SIZE=4096ull)

### Library Installation ###

# If you know the plugin directory of your MySQL installation, you can uncomment
# the following line and set it here.
# set(MYSQL_PLUGIN_DIR "c:\\path\\to\\plugin\\dir")

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
## Tests
add_executable(tests tests/doctest.h common.h tests/testharness.hpp tests/testcases.cpp
		damlevconst.cpp)
target_compile_definitions(tests PRIVATE LEV_FUNCTION=damlevconst)

## Benchmark
add_executable(benchmark common.h tests/testharness.hpp damlevconst.cpp noop.cpp tests/benchmark.cpp)
# Boost is only required for the benchmark target.
# ToDo: Use FindPackage.
target_include_directories(benchmark PUBLIC /usr/local/include)
target_compile_definitions(benchmark PRIVATE WORD_COUNT=235000ul)
target_compile_definitions(benchmark PRIVATE BENCH_FUNCTION=damlevconst)
target_compile_definitions(benchmark PRIVATE WORDS_PATH="/usr/share/dict/words")

## This custom target actually runs the benchmark.
#add_custom_target(benchmark DEPENDS damlevconst)
## iosifovitch2 iosifovitch_original dynamic 8bitchars
## Set this higher for a longer run.
#set(BENCHMARK_WORDS 1000)
#add_custom_command(TARGET benchmark
#		POST_BUILD
#		COMMAND time echo ${BENCHMARK_WORDS} | ./iosifovitch_experiment
#		COMMAND time echo ${BENCHMARK_WORDS} | ./iosifovitch
#		#        COMMAND time echo ${BENCHMARK_WORDS} | ./iosifovitch2
#		COMMAND time echo ${BENCHMARK_WORDS} | ./iosifovitch3
#		#        COMMAND time echo ${BENCHMARK_WORDS} | ./iosifovitch_original
#		#        COMMAND time echo ${BENCHMARK_WORDS} | ./dynamic
#		# COMMAND time echo ${BENCHMARK_WORDS} | ./classic
#		# COMMAND time echo ${BENCHMARK_WORDS} | ./8bitchars
#		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
#		)
