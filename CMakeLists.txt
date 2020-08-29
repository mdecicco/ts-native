cmake_minimum_required(VERSION 3.10)
project(gjs)

set(
	include_dirs
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/deps/asmjit/src
)
set(
	library_dirs
	${CMAKE_CURRENT_SOURCE_DIR}/deps/asmjit/Deps
)
include_directories(${include_dirs})
link_directories(${library_dirs})

add_subdirectory("./deps")

file(GLOB_RECURSE gjs_src
	"src/*.h"
	"src/*.hpp"
	"src/*.cpp"
	"src/compiler/*.h"
	"src/compiler/*.hpp"
	"src/compiler/*.cpp"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_executable(gjs ${gjs_src})

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

target_link_libraries(gjs asmjit)

set_target_properties(gjs PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/bin 
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
