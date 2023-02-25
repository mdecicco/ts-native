@echo off
set /p test_name="Enter test name: "
copy default_test.txt "./%test_name%.cpp" >nul
copy /b CMakeLists.txt +,,