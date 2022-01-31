cmake_minimum_required(VERSION 3.10)
project(playground)

set(
    include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../examples/deps/imgui
)
set(
    library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)
include_directories(${include_dirs})
link_directories(${library_dirs})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB playground_src
    "./*.h"
    "./*.hpp"
    "./*.cpp"
)
file(GLOB playground_scripts "bin/*.gjs")

SOURCE_GROUP("src" FILES ${playground_src})
SOURCE_GROUP("scripts" FILES ${playground_scripts})

add_executable(playground ${playground_src} ${playground_scripts})
set(libs gjs glfw imgui)

if (MSVC)
    set (libs ${libs} opengl32)
else()
    find_package(OpenGL REQUIRED)
    set (libs ${libs} ${OPENGL_LIBRARIES})
endif()

target_link_libraries(playground ${libs})

set_target_properties(playground PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/bin 
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
