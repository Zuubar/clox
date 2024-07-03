file(GLOB SOURCES "./*.c")
list(REMOVE_ITEM ${SOURCES} "main.c")

option(DEBUG_PRINT_CODE "Enable debug mode" OFF)
option(DEBUG_TRACE_EXECUTION "Enable debug trace mode" OFF)

add_library(clox_lib ${SOURCES})
if (DEBUG_PRINT_CODE)
    target_compile_definitions(clox_lib PRIVATE DEBUG_PRINT_CODE)
endif ()

if (DEBUG_TRACE_EXECUTION)
    target_compile_definitions(clox_lib PRIVATE DEBUG_TRACE_EXECUTION)
endif ()

target_link_libraries(clox_lib m)

add_executable(clox main.c)
target_link_libraries(clox clox_lib)
