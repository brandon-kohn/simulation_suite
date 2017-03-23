@echo off
REM `simulation_suite` update and build script.
REM This script does a pull on simulation_suite at the URL below.
REM It will then invoke cmake to update and build the suite installing into the specified path.
REM Requires git and cmake on the path.

if "%~1"=="" goto usage
if "%~2"=="" goto usage

REM Update the repo
SET REPOSRC=https://github.com/brandon-kohn/simulation_suite.git

git pull "%REPOSRC%"
git submodule update --init --recursive

SET INSTALL_PATH=%1
SET CMAKE_GENERATOR=%2

REM Build the release
cmake -H. -Bcmake.build -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX="%INSTALL_PATH%" -G%CMAKE_GENERATOR%
cmake --build "cmake.build" --target install

REM Build the debug
cmake -H. -Bcmake.build -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX="%INSTALL_PATH%" -G%CMAKE_GENERATOR%
cmake --build "cmake.build" --target install
goto :eof

:usage
@echo Usage: update.bat ^<Install Path Prefix^> ^<"CMAKE Generator"^>
exit /B 1
REM End