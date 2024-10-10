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

# Modify the following to set the buffer size to something other than 4096
# characters. There is a hard max set at ~16KB. This is the wrong tool for
# the job if you are using it for strings that large.
set(BUFFER_SIZE 4096ull)

# Select exactly one policy for handling negative max edit distance
add_compile_definitions(RETURN_ZERO_ON_BAD_MAX)
#add_compile_definitions(RETURN_NULL_ON_BAD_MAX)

# Select exactly one policy for handling strings that require a buffer size greater than the allocated buffer.
add_compile_definitions(TRUNCATE_ON_BUFFER_EXCEEDED) # Truncates the strings to fit within the buffer.
#add_compile_definitions(RETURN_ZERO_ON_BUFFER_EXCEEDED)
#add_compile_definitions(RETURN_NULL_ON_BUFFER_EXCEEDED)

# If you know the plugin directory of your MySQL installation, you can uncomment
# the following line and set it here. Otherwise, we attempt to get the path by
# executing `mysql_config --include`.
#set(MYSQL_PLUGIN_DIR "/opt/homebrew/opt/mysql/lib/plugin/")

# Likewise for the location of `mysql.h`, the MySQL include directory.
#set(MYSQL_INCLUDE "/opt/homebrew/include/mysql/")

# Use this file for a list of words for testing/benchmarking.
set(WORDS_FILE "/usr/share/dict/words")

# END CONFIGURATION


# MySQL Include Directory
if(NOT DEFINED MYSQL_INCLUDE)
    execute_process(COMMAND mysql_config --include
            OUTPUT_VARIABLE MYSQL_INCLUDE
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Fallback hail-Mary for Win64
    if(WIN64 AND NOT DEFINED MYSQL_INCLUDE)
        set(MYSQL_INCLUDE "-I C:/Program Files/MySQL/MySQL Server 8.0/include")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MYSQL_INCLUDE}")
message(STATUS "Include dir: ${MYSQL_INCLUDE}")
message(STATUS "CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# Determine MYSQL_PLUGIN_DIR using mysql_config
if(NOT DEFINED MYSQL_PLUGIN_DIR)
    if(INSTALL_PLUGINDIR)
        set(MYSQL_PLUGIN_DIR ${INSTALL_PLUGINDIR})
    else()
        # Note: `mysql_config` must be in the PATH for this to work.
        execute_process(COMMAND mysql_config --plugindir
                OUTPUT_VARIABLE MYSQL_PLUGIN_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()

    # Fallback hail-Mary for Win64
    if(WIN64 AND NOT DEFINED MYSQL_PLUGIN_DIR)
        set(MYSQL_PLUGIN_DIR "C:\\Program Files\\MySQL\\MySQL Server 8.0\\lib\\plugin")
    endif()
endif()
message(STATUS "Plugin dir: ${MYSQL_PLUGIN_DIR}")


# Report configured values.
message(STATUS "Configured BUFFER_SIZE: ${BUFFER_SIZE}")

if(DEFINED MYSQL_PLUGIN_DIR)
    message(STATUS "Configured MYSQL_PLUGIN_DIR: ${MYSQL_PLUGIN_DIR}")
else()
    message(STATUS "MYSQL_PLUGIN_DIR is not set; installation will likely fail.")
endif()

if(DEFINED MYSQL_INCLUDE)
    message(STATUS "Configured MYSQL_INCLUDE: ${MYSQL_INCLUDE}")
else()
    message(STATUS "MYSQL_INCLUDE is not set; building will likely fail.")
endif()

message(STATUS "Configured WORDS_FILE for testing/benchmark: ${WORDS_FILE}")

# Report selected policies
if(RETURN_ZERO_ON_BAD_MAX)
    message(STATUS "Policy for handling negative max edit distance: RETURN_ZERO_ON_BAD_MAX")
elseif(RETURN_NULL_ON_BAD_MAX)
    message(STATUS "Policy for handling negative max edit distance: RETURN_NULL_ON_BAD_MAX")
endif()

if(TRUNCATE_ON_BUFFER_EXCEEDED)
    message(STATUS "Policy for handling buffer size exceeded: TRUNCATE_ON_BUFFER_EXCEEDED")
elseif(RETURN_ZERO_ON_BUFFER_EXCEEDED)
    message(STATUS "Policy for handling buffer size exceeded: RETURN_ZERO_ON_BUFFER_EXCEEDED")
elseif(RETURN_NULL_ON_BUFFER_EXCEEDED)
    message(STATUS "Policy for handling buffer size exceeded: RETURN_NULL_ON_BUFFER_EXCEEDED")
endif()


# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
