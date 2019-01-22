cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_CXX_STANDARD 17)

message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

project(Levenshtein)

#set(CMAKE_BUILD_TYPE Release)
#SET(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall -pedantic -Wextra")

# Library specific.
set(DAMLEV_BUFFER_SIZE 500)
add_definitions("-DHAVE_DLOPEN")

add_library(damlev MODULE Levenshtein.cpp)

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
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${MYSQL_INCLUDE}")
message(STATUS "Include dir: ${MYSQL_INCLUDE}")
message(STATUS "C_FLAGS: ${CMAKE_CXX_FLAGS}")

install(TARGETS damlev LIBRARY DESTINATION ${MYSQL_PLUGIN_DIR})
