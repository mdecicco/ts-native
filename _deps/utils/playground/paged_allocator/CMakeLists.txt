project(paged_allocator_pg)

file(GLOB_RECURSE 2_paged_allocator_pg_src "*.h" "*.cpp")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_executable(2_paged_allocator_pg ${2_paged_allocator_pg_src})
set_property(TARGET 2_paged_allocator_pg PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
 
SOURCE_GROUP("" FILES ${2_paged_allocator_pg_src})

target_include_directories(2_paged_allocator_pg PUBLIC ../../include)
target_link_libraries(2_paged_allocator_pg utils)
set_target_properties(2_paged_allocator_pg PROPERTIES FOLDER playgrounds)
