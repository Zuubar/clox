file(GLOB SOURCES "./*.c")
list(REMOVE_ITEM ${SOURCES} "main.c")

add_library(clox_lib ${SOURCES})
target_link_libraries(clox_lib m)

add_executable(clox main.c)
target_link_libraries(clox clox_lib)
