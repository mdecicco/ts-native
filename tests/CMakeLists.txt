cmake_minimum_required(VERSION 3.10)
project(gjs_test)

set(
    include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
include_directories(${include_dirs})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
file(GLOB_RECURSE gjs_test_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)
SOURCE_GROUP("src" FILES ${gjs_test_src})

add_executable(gjs_test ${gjs_test_src})

target_link_libraries(gjs_test gjs)

set_target_properties(gjs_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/bin 
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
