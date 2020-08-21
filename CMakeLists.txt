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

option(GJS_BIG_INTS "Use 64-bit script integers")
option(GJS_BIG_FLOATS "Use 64-bit script floats")

add_subdirectory("./deps")

file(GLOB src
	"src/*.h"
	"src/*.hpp"
	"src/*.cpp"
)
source_group(src FILES ${src})

add_executable(gjs ${src})

set(defines )

if (GJS_BIG_INTS)
	target_compile_definitions(gjs PRIVATE "SCRIPTS_USE_64BIT_INTEGERS=1")
endif ()

if (GJS_BIG_FLOATS)
	target_compile_definitions(gjs PRIVATE "SCRIPTS_USE_64BIT_DECIMALS=1")
endif ()

target_link_libraries(gjs asmjit)

set_target_properties(gjs PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/bin 
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
