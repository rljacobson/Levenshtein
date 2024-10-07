# src/CMakeLists.txt

# Define library sources
set(DAMLEV_SOURCES
        algorithms/damlev.cpp
        algorithms/damlevp.cpp
        algorithms/damlevlim.cpp
        algorithms/damlevconst.cpp
        algorithms/damlevconstmin.cpp
        algorithms/damlev2D.cpp
        algorithms/noop.cpp
)

# Modify the buffer size if needed (default is 4096ull)
set(BUFFER_SIZE 4096ull)

# Add the damlev library as SHARED
add_library(damlev SHARED ${DAMLEV_SOURCES})

# Set compile definitions
target_compile_definitions(damlev PRIVATE
        WORDS_PATH="/usr/share/dict/words"
        MYSQL_DYNAMIC_PLUGIN
        DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
)

# Include directories with PUBLIC scope to propagate to linked targets
target_include_directories(damlev PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${PROJECT_SOURCE_DIR}/include
        /opt/homebrew/Cellar/mysql/8.3.0_1/include/mysql
)

### Library Installation ###

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
endif()
message(STATUS "Plugin dir: ${MYSQL_PLUGIN_DIR}")

# Install the damlev library
install(TARGETS damlev LIBRARY DESTINATION ${MYSQL_PLUGIN_DIR})
