cmake_minimum_required(VERSION 3.10)
project(boids)

set(
    include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/imgui
)
set(
    library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
)
include_directories(${include_dirs})
link_directories(${library_dirs})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB boids_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)
file(GLOB boids_scripts "bin/*.gjs")

SOURCE_GROUP("src" FILES ${boids_src})
SOURCE_GROUP("scripts" FILES ${boids_scripts})

add_executable(boids ${boids_src} ${boids_scripts})

target_link_libraries(
    boids
    gjs
    glfw
    imgui
    opengl32
)

set_target_properties(boids PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/bin 
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin
)