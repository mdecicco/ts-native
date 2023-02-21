@echo off
set /p test_name="Enter test name: "
copy default_test.cpp "./tests/%test_name%.cpp" >nul
cd tests
copy /b CMakeLists.txt +,,
cd ..