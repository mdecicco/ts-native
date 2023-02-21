@echo off

cd playground

set pg_number=
for /d %%a in (.\*) do set /a pg_number+=1

set /p pg_name="Enter playground name: "

set project_name=%pg_name%_test
set executable_name=%pg_number%_%pg_name%_pg

echo add_subdirectory(%pg_name%)>> CMakeLists.txt

mkdir %pg_name%
copy default_main.cpp "./%pg_name%/main.cpp" >nul
cd %pg_name%

echo project(%pg_name%_pg)>> CMakeLists.txt
echo.>> CMakeLists.txt
echo file(GLOB_RECURSE %executable_name%_src "*.h" "*.cpp")>> CMakeLists.txt
echo set_property(GLOBAL PROPERTY USE_FOLDERS ON)>> CMakeLists.txt
echo add_executable(%executable_name% ${%executable_name%_src})>> CMakeLists.txt
echo set_property(TARGET %executable_name% PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")>> CMakeLists.txt
echo. >> CMakeLists.txt
echo SOURCE_GROUP("" FILES ${%executable_name%_src})>> CMakeLists.txt
echo.>> CMakeLists.txt
echo target_include_directories(%executable_name% PUBLIC ../../include)>> CMakeLists.txt
echo target_link_libraries(%executable_name% r3)>> CMakeLists.txt
echo set_target_properties(%executable_name% PROPERTIES FOLDER playgrounds)

pause