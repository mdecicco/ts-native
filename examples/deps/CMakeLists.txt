add_subdirectory(glfw)

cmake_minimum_required(VERSION 3.10)
project(imgui)

set(
    include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
)
include_directories(${include_dirs})

file(GLOB imgui_src
    "./imgui/*.h"
    "./imgui/*.hpp"
    "./imgui/*.cpp"
    "./imgui/backends/imgui_impl_glfw.h"
    "./imgui/backends/imgui_impl_glfw.cpp"
    "./imgui/backends/imgui_impl_opengl3.h"
    "./imgui/backends/imgui_impl_opengl3.cpp"
)

SOURCE_GROUP("src" FILES ${imgui_src})

add_library(imgui ${imgui_src})

set_target_properties(imgui PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/bin 
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin
  FOLDER "imgui"
)