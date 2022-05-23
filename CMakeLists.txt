cmake_minimum_required(VERSION 3.0.0)
project(gs VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF) 

set(
    gs_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/_deps/utils/include
    ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/libffi/include
)
set(
    gs_library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/_bin/Debug
)

set (all_sources "")
macro(add_sources group dir)
    set(_all "")

    file(GLOB _inc "include/gs/${dir}/*.h" "include/gs/${dir}/*.hpp" "include/gs/${dir}/*.inl")
    list(APPEND _all ${_inc})

    file(GLOB _src "src/${dir}/*.cpp" "src/${dir}/*.c")
    list(APPEND _all ${_src})

    list(APPEND all_sources ${_all})
    string(REPLACE "/" "\\" group_path "${group}")
    source_group(${group_path} FILES ${_all})

    unset(_inc)
    unset(_src)
    unset(_all)
endmacro(add_sources)

add_sources("src"            ".")
add_sources("src/bind"       "bind")
add_sources("src/common"     "common")
add_sources("src/interfaces" "interfaces")
add_sources("src/utils"      "utils")
add_sources("src/compiler"   "compiler")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/_bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/_bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/_bin)


include_directories(${gs_include_dirs})
link_directories(${gs_library_dirs})

add_library(gs ${all_sources})
target_link_libraries(gs
    utils
    ffi_static
)

add_subdirectory("./_deps")

set(GS_BUILD_TESTS TRUE CACHE BOOL "Build gs test targets")
set(GS_BUILD_PLAYGROUND TRUE CACHE BOOL "Build gs playground")
set(GS_BUILD_TOOLS TRUE CACHE BOOL "Build gs tools")

if (GS_BUILD_TESTS)
    add_subdirectory("./_tests")
endif ()

if (GS_BUILD_PLAYGROUND)
    add_subdirectory("./_playground")
endif ()

if (GS_BUILD_TOOLS)
    add_subdirectory("./_tools")
endif ()