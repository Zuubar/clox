cmake_minimum_required(VERSION 3.0)
project(clox C)

set(CMAKE_C_STANDARD 11)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

add_executable(clox src/main.c
        src/memory.c
        src/debug.c
        src/chunk.c
        src/value.c
        src/line.c
        src/vm.c
        src/compiler.c
        src/scanner.c
        src/object.c
        src/table.c
)
