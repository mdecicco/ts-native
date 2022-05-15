project(array_pg)

file(GLOB_RECURSE 3_array_pg_src "*.h" "*.cpp")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_executable(3_array_pg ${3_array_pg_src})
set_property(TARGET 3_array_pg PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
 
SOURCE_GROUP("" FILES ${3_array_pg_src})

target_include_directories(3_array_pg PUBLIC ../../include)
target_link_libraries(3_array_pg utils)
set_target_properties(3_array_pg PROPERTIES FOLDER playgrounds)