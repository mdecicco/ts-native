cmake_minimum_required(VERSION 3.0.0)
project(tsnr)

set(
    tsnr_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB_RECURSE tsnr_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)

SOURCE_GROUP("src" FILES ${tsnc_src})

include_directories(${tsnr_include_dirs})
add_executable(tsnr ${tsnr_src})

target_link_libraries(tsnr tsn)
