cmake_minimum_required(VERSION 3.10)
project(gjs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(
    gjs_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/dyncall/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/asmjit/src
)
set(
    gjs_library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/dyncall/lib
)
include_directories(${gjs_include_dirs})
link_directories(${gjs_library_dirs})

set (all_sources "")
macro(add_sources group dir)
    set(_all "")

    file(GLOB _inc "include/gjs/${dir}/*.h" "include/gjs/${dir}/*.hpp" "include/gjs/${dir}/*.inl")
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
add_sources("src/optimize"  "optimize")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(gjs ${all_sources})

add_subdirectory("./deps")

set(GJS_BUILD_TESTS TRUE CACHE BOOL "Build gjs test targets")
set(GJS_BUILD_PLAYGROUND TRUE CACHE BOOL "Build gjs playground target")
set(GJS_BUILD_EXAMPLES TRUE CACHE BOOL "Build gjs example targets")
set(GJS_BUILD_CLI TRUE CACHE BOOL "Build gjs CLI target")

if (GJS_BUILD_TESTS)
    add_subdirectory("./tests")
endif ()

if (GJS_BUILD_PLAYGROUND)
add_subdirectory("./playground")
endif ()

if (GJS_BUILD_EXAMPLES)
add_subdirectory("./examples")
endif ()

if (GJS_BUILD_CLI)
add_subdirectory("./cli")
endif ()


target_link_libraries(gjs
    dyncall_s
    asmjit
)

set_target_properties(dyncall_s PROPERTIES FOLDER dependencies)
set_target_properties(dynload_s PROPERTIES FOLDER dependencies)
set_target_properties(dyncallback_s PROPERTIES FOLDER dependencies)