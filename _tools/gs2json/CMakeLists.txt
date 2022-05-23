cmake_minimum_required(VERSION 3.0.0)
project(gs2json)

set(
    gs2json_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set(
    gs2json_library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../_lib/Debug
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB_RECURSE gs2json_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)

SOURCE_GROUP("src" FILES ${gs2json_src})


include_directories(${gs2json_include_dirs})
link_directories(${gs2json_library_dirs})
add_executable(gs2json ${gs2json_src})

target_link_libraries(gs2json gs ffi_shared)