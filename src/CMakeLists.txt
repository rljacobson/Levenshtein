### Targets ###

# Define library sources
set(DAMLEV_SOURCES
        algorithms/damlev.cpp
        algorithms/damlev2D.cpp
        algorithms/damlevlim.cpp
        algorithms/damlevmin.cpp
        algorithms/damlevminp.cpp
        algorithms/damlevp.cpp
        algorithms/lev.cpp
        algorithms/levlim.cpp
        algorithms/levlimopt.cpp
        algorithms/noop.cpp
        algorithms/printmatrix.cpp
)


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

