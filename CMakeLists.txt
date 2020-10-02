cmake_minimum_required(VERSION 3.10)
project(gjs)

set(
    include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/asmjit/src
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/dyncall/include
)
set(
    library_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/dyncall/lib
)
include_directories(${include_dirs})
link_directories(${library_dirs})

add_subdirectory("./deps")
add_subdirectory("./tests")
add_subdirectory("./playground")

file(GLOB_RECURSE gjs_src
    "src/*.h"
    "src/*.hpp"
    "src/*.cpp"
    "src/compiler/*.h"
    "src/compiler/*.hpp"
    "src/compiler/*.cpp"
    "src/lexer/*.h"
    "src/lexer/*.hpp"
    "src/lexer/*.cpp"
    "src/parser/*.h"
    "src/parser/*.hpp"
    "src/parser/*.cpp"
    "src/vm/*.h"
    "src/vm/*.hpp"
    "src/vm/*.cpp"
    "src/backends/*.h"
    "src/backends/*.hpp"
    "src/backends/*.cpp"
    "src/util/*.h"
    "src/util/*.hpp"
    "src/util/*.cpp"
    "src/bind/*.h"
    "src/bind/*.hpp"
    "src/bind/*.cpp"
    "src/common/*.h"
    "src/common/*.hpp"
    "src/common/*.cpp"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(gjs ${gjs_src})

MACRO(SOURCE_GROUP_BY_FOLDER target)
  SET(SOURCE_GROUP_DELIMITER "/")
  SET(last_dir "")
  SET(files "")
  FOREACH(file ${${target}_src})
    file(RELATIVE_PATH relative_file "${PROJECT_SOURCE_DIR}" ${file})
    GET_FILENAME_COMPONENT(dir "${relative_file}" PATH)
    IF (NOT "${dir}" STREQUAL "${last_dir}")
      IF (files)
        SOURCE_GROUP("${last_dir}" FILES ${files})
      ENDIF (files)
      SET(files "")
    ENDIF (NOT "${dir}" STREQUAL "${last_dir}")
    SET(files ${files} ${file})
    SET(last_dir "${dir}")
  ENDFOREACH(file)
  IF (files)
    SOURCE_GROUP("${last_dir}" FILES ${files})
  ENDIF (files)
ENDMACRO(SOURCE_GROUP_BY_FOLDER)

SOURCE_GROUP_BY_FOLDER(gjs)

target_link_libraries(gjs asmjit dyncall_s)
add_custom_command(TARGET gjs COMMAND robocopy ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include *.h -MIR)
