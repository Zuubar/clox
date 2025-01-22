file(GLOB SOURCES "./test_*.c")

function(create_test_executable test_file)
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file} common.c)
    target_link_libraries(${test_name} unity libclox)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

foreach(test_file ${SOURCES})
    create_test_executable(${test_file})
endforeach()
