# Set the target system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)

# Specify the cross compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# Target architecture
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify where to find libraries and includes
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 /usr/local/lib/mysql-windows)

# Ensure programs are found in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and includes only in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#set(MYSQL_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include/mysql-8-0-39")