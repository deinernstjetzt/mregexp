cmake_minimum_required(VERSION 3.20)
project(mregexp C)

enable_testing()

set(CMAKE_C_STANDARD 17)

add_library(mregexp src/mregexp.c src/mregexp.h)
target_include_directories(mregexp PUBLIC src)

if ("${BUILD_TESTING}" STREQUAL "ON")
    function(add_unit_test name)
        add_executable(${name} tests/${name}.c)
        target_link_libraries(${name} mregexp)
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    add_unit_test(basic_string_matching)
    add_unit_test(parse_utf8)
endif()
