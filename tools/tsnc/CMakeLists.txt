cmake_minimum_required(VERSION 3.0.0)
project(tsnc)

set(
    tsnc_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB_RECURSE tsnc_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)

SOURCE_GROUP("src" FILES ${tsnc_src})

include_directories(${tsnc_include_dirs})
add_executable(tsnc ${tsnc_src})

target_link_libraries(tsnc tsn)