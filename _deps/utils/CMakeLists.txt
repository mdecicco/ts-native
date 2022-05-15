cmake_minimum_required(VERSION 3.10) 
project(utils)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF) 

set( 
    utils_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
set( 
    utils_library_dirs 
    ""
)

set (all_utils_sources "") 
macro(add_sources group dir) 
    set(_all "") 
    
    file(GLOB _inc "include/utils/${dir}/*.h" "include/utils/${dir}/*.hpp" "include/utils/${dir}/*.inl") 
    list(APPEND _all ${_inc}) 
    
    file(GLOB _src "src/${dir}/*.cpp") 
    list(APPEND _all ${_src}) 
    
    list(APPEND all_utils_sources ${_all}) 
    string(REPLACE "/" "\\" group_path "${group}") 
    source_group(${group_path} FILES ${_all}) 
    
    unset(_inc) 
    unset(_src) 
    unset(_all) 
endmacro(add_sources) 

add_sources("src" ".") 

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(${utils_include_dirs}) 
link_directories(${utils_library_dirs}) 
add_library(utils ${all_utils_sources}) 

set(UTILS_BUILD_PLAYGROUNDS TRUE CACHE BOOL "Build utils playground targets")

if (UTILS_BUILD_PLAYGROUNDS)
    add_subdirectory("./playground") 
endif()

if (CMAKE_VERSION VERSION_GREATER 3.7.8)
    if (MSVC_IDE)
        option(VS_ADD_NATIVE_VISUALIZERS "Configure project to use Visual Studio native visualizers" TRUE)
    else()
        set(VS_ADD_NATIVE_VISUALIZERS FALSE CACHE INTERNAL "Native visualizers are a Visual Studio extension" FORCE)
    endif()

    # add natvis file to the library so it will automatically be loaded into Visual Studio
    if (VS_ADD_NATIVE_VISUALIZERS)
        target_sources(utils INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/debug.natvis
        )
    endif()
endif()