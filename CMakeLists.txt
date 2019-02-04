cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_CXX_STANDARD 17)

message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

project(Levenshtein)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall -pedantic -Wextra")

# Library specific.
#add_definitions("-DHAVE_DLOPEN")

set(DAMLEV_SOURCES
        damlev.cpp
        damlevp.cpp
        damlevlim.cpp
        damlevlimp.cpp
   )


add_library(damlev MODULE ${DAMLEV_SOURCES})
# Uncomment the following to set the buffer size to something other than 512
# characters. There is a hard max set at ~16KB. This is the wrong tool for
# the job if you are using it for strings that large.
#   target_compile_definitions(damlev PRIVATE DAMLEV_BUFFER_SIZE=4096ull)



# Library Installation

if(NOT DEFINED MYSQL_PLUGIN_DIR)
    if(INSTALL_PLUGINDIR)
        set(MYSQL_PLUGIN_DIR ${INSTALL_PLUGINDIR})
    else()
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
		set(MYSQL_INCLUDE "C:\Program Files\MySQL\MySQL Server 8.0\include")
	endif()
	if(NOT DEFINED MYSQL_PLUGIN_DIR)
		set(MYSQL_PLUGIN_DIR "C:\Program Files\MySQL\MySQL Server 8.0\lib\plugin")
	endif()
endif()
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${MYSQL_INCLUDE}")
message(STATUS "Include dir: ${MYSQL_INCLUDE}")
message(STATUS "C_FLAGS: ${CMAKE_CXX_FLAGS}")

install(TARGETS damlev LIBRARY DESTINATION ${MYSQL_PLUGIN_DIR})
