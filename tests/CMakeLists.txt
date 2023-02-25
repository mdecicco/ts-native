project(tsn_unit_tests)
add_subdirectory(deps/catch2)

file(GLOB tsn_test_sources "./*.cpp")
add_executable(tsn_tests ${tsn_test_sources})
set_property(TARGET tsn_tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
 
target_include_directories(tsn_tests PUBLIC ../include)
target_link_libraries(tsn_tests PRIVATE Catch2::Catch2WithMain tsn)