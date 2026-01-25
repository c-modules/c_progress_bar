file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "test*.c")

foreach(SOURCE_FILE ${TEST_SOURCES})

    get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)

    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE c_progress_bar::c_progress_bar)

    if(ENABLE_SANITIZERS AND NOT MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address,undefined)
        target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address,undefined)
    endif()

    # Test
    add_test(
        NAME ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} 
            -DTEST_PROG=$<TARGET_FILE:${TARGET_NAME}>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_test.cmake
    )

endforeach()