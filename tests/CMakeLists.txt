file(GLOB SOURCES "./*.c")
add_executable(clox_tests ${SOURCES})
target_link_libraries(clox_tests unity clox_lib)
