### Targets ###

# Define library sources
set(DAMLEV_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/damlev.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/damlev2D.cpp // Only used for testing/debugging
        ${CMAKE_CURRENT_SOURCE_DIR}/damlevlim.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/damlevmin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/damlevminp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/damlevp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lev.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/levlim.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/levlimopt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/levmin.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/noop.cpp
)
# CMake is an abomination
set(DAMLEV_SOURCES ${DAMLEV_SOURCES} PARENT_SCOPE)


# Add the damlev library as SHARED
add_library(damlev SHARED ${DAMLEV_SOURCES})
target_compile_options(damlev PRIVATE -O3 -ffast-math)

# Set compile definitions
target_compile_definitions(damlev PRIVATE
        WORDS_PATH=${WORDS_FILE}
        MYSQL_DYNAMIC_PLUGIN
        DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
)

# Install the damlev library
install(TARGETS damlev LIBRARY DESTINATION ${MYSQL_PLUGIN_DIR})

