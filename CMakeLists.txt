cmake_minimum_required(VERSION 3.0)
project(clox C)

set(CMAKE_C_STANDARD 11)

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
        src/buffer.c
)

target_link_libraries(clox m)
