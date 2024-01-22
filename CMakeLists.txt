cmake_minimum_required(VERSION 3.0.0)
project(tsn VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(
    tsn_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/utils/include
    ${CMAKE_CURRENT_SOURCE_DIR}/build/deps/libffi/include
)

set (all_sources "")
macro(add_sources group dir)
    set(_all "")

    file(GLOB _inc "include/tsn/${dir}/*.h" "include/tsn/${dir}/*.hpp" "include/tsn/${dir}/*.inl")
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
add_sources("src/io"         "io")
add_sources("src/vm"         "vm")
add_sources("src/ffi"        "ffi")
add_sources("src/bind"       "bind")
add_sources("src/utils"      "utils")
add_sources("src/common"     "common")
add_sources("src/builtin"    "builtin")
add_sources("src/backend"    "backend")
add_sources("src/compiler"   "compiler")
add_sources("src/optimize"   "optimize")
add_sources("src/pipeline"   "pipeline")
add_sources("src/interfaces" "interfaces")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(${tsn_include_dirs})

add_library(tsn ${all_sources})
target_link_libraries(tsn
    utils
    ffi_static
)

add_subdirectory("./deps")

set(TSN_BUILD_TESTS FALSE CACHE BOOL "Build tsn test targets")
set(TSN_BUILD_TOOLS TRUE CACHE BOOL "Build tsn tools")

if (TSN_BUILD_TESTS)
    add_subdirectory("./tests")
endif ()

if (TSN_BUILD_TOOLS)
    add_subdirectory("./tools")
endif ()
