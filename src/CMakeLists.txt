file(GLOB SOURCES "./*.c")
list(REMOVE_ITEM ${SOURCES} "main.c")

option(NAN_BOXING "Enable NAN boxing optimization" ON)
option(DEBUG_PRINT_CODE "Enable debug mode" OFF)
option(DEBUG_TRACE_EXECUTION "Enable trace execution" OFF)
option(DEBUG_STRESS_GC "Enable stressing garbage collector" OFF)
option(DEBUG_LOG_GC "Enable logging garbage collector" OFF)

add_library(libclox ${SOURCES})
if (NAN_BOXING)
    target_compile_definitions(libclox PRIVATE NAN_BOXING)
endif ()

if (DEBUG_PRINT_CODE)
    target_compile_definitions(libclox PRIVATE DEBUG_PRINT_CODE)
endif ()

if (DEBUG_TRACE_EXECUTION)
    target_compile_definitions(libclox PRIVATE DEBUG_TRACE_EXECUTION)
endif ()

if (DEBUG_STRESS_GC)
    target_compile_definitions(libclox PRIVATE DEBUG_STRESS_GC)
endif ()

if (DEBUG_LOG_GC)
    target_compile_definitions(libclox PRIVATE DEBUG_LOG_GC)
endif ()

target_link_libraries(libclox m)

add_executable(clox main.c)
target_link_libraries(clox libclox)
