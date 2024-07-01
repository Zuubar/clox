cmake_minimum_required(VERSION 3.10)
project(clox C)

set(CMAKE_C_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(src)
add_subdirectory(unity)
add_subdirectory(tests)
