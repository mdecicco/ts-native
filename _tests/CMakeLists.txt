cmake_minimum_required(VERSION 3.0.0)
project(tsn_tests)

set(
    tsn_tests_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set(
    tsn_tests_library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB_RECURSE tsn_tests_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)

SOURCE_GROUP("src" FILES ${tsn_tests_src})


include_directories(${tsn_tests_include_dirs})
link_directories(${tsn_tests_library_dirs})
add_executable(tsn_tests ${tsn_tests_src})

target_link_libraries(tsn_tests tsn ffi_shared)