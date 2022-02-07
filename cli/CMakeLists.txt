cmake_minimum_required(VERSION 3.10)
project(cli)

set(
    include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
set(
    library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)
include_directories(${include_dirs})
link_directories(${library_dirs})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB cli_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)

file(GLOB cli_scripts "bin/*.gjs")

SOURCE_GROUP("src" FILES ${cli_src})
SOURCE_GROUP("scripts" FILES ${cli_scripts})

add_executable(cli ${cli_src} ${cli_scripts})

target_link_libraries(
    cli
    gjs
)

set_target_properties(cli PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/bin 
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin
)