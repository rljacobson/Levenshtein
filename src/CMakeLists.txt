### Targets ###

# Define library sources
set(DAMLEV_SOURCES
        algorithms/damlev.cpp
        algorithms/damlevp.cpp
        algorithms/damlevmin.cpp
        algorithms/damlevlim.cpp
        algorithms/damlevlimopt.cpp
        algorithms/damlev2D.cpp
        algorithms/noop.cpp
)


# Add the damlev library as SHARED
add_library(damlev SHARED ${DAMLEV_SOURCES})

# Set compile definitions
target_compile_definitions(damlev PRIVATE
        WORDS_PATH=${WORDS_FILE}
        MYSQL_DYNAMIC_PLUGIN
        DAMLEV_BUFFER_SIZE=${BUFFER_SIZE}
)

# Install the damlev library
install(TARGETS damlev LIBRARY DESTINATION ${MYSQL_PLUGIN_DIR})

