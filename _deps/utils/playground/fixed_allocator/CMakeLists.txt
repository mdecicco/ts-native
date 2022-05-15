project(fixed_allocator_pg)

file(GLOB_RECURSE 1_fixed_allocator_pg_src "*.h" "*.cpp")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_executable(1_fixed_allocator_pg ${1_fixed_allocator_pg_src})
set_property(TARGET 1_fixed_allocator_pg PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
 
SOURCE_GROUP("" FILES ${1_fixed_allocator_pg_src})

target_include_directories(1_fixed_allocator_pg PUBLIC ../../include)
target_link_libraries(1_fixed_allocator_pg utils)
set_target_properties(1_fixed_allocator_pg PROPERTIES FOLDER playgrounds)
