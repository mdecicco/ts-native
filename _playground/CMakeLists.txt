cmake_minimum_required(VERSION 3.0.0)
project(tsn_playground)

set(
    tsn_playground_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set(
    tsn_playground_library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../_lib/Debug
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB_RECURSE tsn_playground_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)

SOURCE_GROUP("src" FILES ${tsn_playground_src})


include_directories(${tsn_playground_include_dirs})
link_directories(${tsn_playground_library_dirs})
add_executable(tsn_playground ${tsn_playground_src})

target_link_libraries(tsn_playground tsn ffi_shared)