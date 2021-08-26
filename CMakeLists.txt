cmake_minimum_required(VERSION 3.10)
project(gjs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(
    include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/dyncall/include
)
set(
    library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/dyncall/lib
)
include_directories(${include_dirs})
link_directories(${library_dirs})

set (all_sources "")
macro(add_sources group dir)
    set(_all "")

    file(GLOB _inc "include/gjs/${dir}/*.h" "include/gjs/${dir}/*.hpp")
    list(APPEND _all ${_inc})

    file(GLOB _src "src/${dir}/*.cpp")
    list(APPEND _all ${_src})

    list(APPEND all_sources ${_all})
    string(REPLACE "/" "\\" group_path "${group}")
    source_group(${group_path} FILES ${_all})

    unset(_inc)
    unset(_src)
    unset(_all)
endmacro(add_sources)

add_sources("src"          ".")
add_sources("src/compiler"  "compiler")
add_sources("src/lexer"     "lexer")
add_sources("src/parser"    "parser")
add_sources("src/util"      "util")
add_sources("src/vm"        "vm")
add_sources("src/common"    "common")
add_sources("src/builtin"   "builtin")
add_sources("src/bind"      "bind")
add_sources("src/backends"  "backends")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(gjs ${all_sources})

add_subdirectory("./deps")
add_subdirectory("./tests")
add_subdirectory("./playground")
add_subdirectory("./cli")

target_link_libraries(gjs
    dyncall_s
)
