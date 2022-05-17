cmake_minimum_required(VERSION 3.0.0)
project(gs_tests)

set(
    gs_tests_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set(
    gs_tests_library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB_RECURSE gs_tests_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)

SOURCE_GROUP("src" FILES ${gs_tests_src})


include_directories(${gs_tests_include_dirs})
link_directories(${gs_tests_library_dirs})
add_executable(gs_tests ${gs_tests_src})

target_link_libraries(gs_tests gs ffi_shared)