file(GLOB SOURCES "./*.c")
list(REMOVE_ITEM ${SOURCES} "main.c")

option(NAN_BOXING "Enable NAN boxing optimization" ON)
option(DEBUG_PRINT_CODE "Enable debug mode" OFF)
option(DEBUG_TRACE_EXECUTION "Enable trace execution" OFF)
option(DEBUG_STRESS_GC "Enable stressing garbage collector" OFF)
option(DEBUG_LOG_GC "Enable logging garbage collector" OFF)

add_library(clox_lib ${SOURCES})
if (NAN_BOXING)
    target_compile_definitions(clox_lib PRIVATE NAN_BOXING)
endif ()

if (DEBUG_PRINT_CODE)
    target_compile_definitions(clox_lib PRIVATE DEBUG_PRINT_CODE)
endif ()

if (DEBUG_TRACE_EXECUTION)
    target_compile_definitions(clox_lib PRIVATE DEBUG_TRACE_EXECUTION)
endif ()

if (DEBUG_STRESS_GC)
    target_compile_definitions(clox_lib PRIVATE DEBUG_STRESS_GC)
endif ()

if (DEBUG_LOG_GC)
    target_compile_definitions(clox_lib PRIVATE DEBUG_LOG_GC)
endif ()

target_link_libraries(clox_lib m)

add_executable(clox main.c)
target_link_libraries(clox clox_lib)
