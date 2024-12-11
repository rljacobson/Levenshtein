# Copyright (C) 2019 Robert Jacobson
# Distributed under the MIT License. See License.txt for details.

### Targets ###

# Define library sources
set(DAMLEV_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/bounded_edit_dist.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bounded_edit_dist_t.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/edit_dist.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/edit_dist_t.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/min_edit_dist.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/min_edit_dist_t.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/min_similarity_t.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/postgres.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/similarity_t.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/edit_dist_t_2d.cpp # Only used for testing/debugging
#        ${CMAKE_CURRENT_SOURCE_DIR}/noop.cpp           # Only used for testing/debugging
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

