@echo off
setlocal enabledelayedexpansion

set /p class_name="Enter class name: "

set header_name="%class_name%.h"
set src_name="%class_name%.cpp"
set "cmakefile=./CMakeLists.txt"

copy default_class.h "./include/utils/%header_name%" >nul
cd "./src"

set printf="C:\Program Files\Git\usr\bin\printf.exe"

%printf% "#include <utils/%header_name%>\n\nnamespace utils {\n};" >> %src_name%

cd ../..
copy /b CMakeLists.txt +,,
pause