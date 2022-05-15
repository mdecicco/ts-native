project(utils_tests)
add_subdirectory(deps/catch2)

file(GLOB utils_test_sources "./*.cpp")
add_executable(utils_tests ${utils_test_sources})
set_property(TARGET utils_tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
 
target_include_directories(utils_tests PUBLIC ../include)