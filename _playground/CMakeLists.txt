cmake_minimum_required(VERSION 3.0.0)
project(gs_playground)

set(
    gs_playground_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set(
    gs_playground_library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../_lib/Debug
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB_RECURSE gs_playground_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)

SOURCE_GROUP("src" FILES ${gs_playground_src})


include_directories(${gs_playground_include_dirs})
link_directories(${gs_playground_library_dirs})
add_executable(gs_playground ${gs_playground_src})

target_link_libraries(gs_playground gs ffi_shared)