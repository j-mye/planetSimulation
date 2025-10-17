@echo off
set BUILD_DIR=build

REM 1. Create build directory if it doesn't exist
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM 2. Configure the project with MinGW Makefiles (only if needed)
if not exist %BUILD_DIR%\CMakeCache.txt (
    echo Configuring project with MinGW Makefiles...
    cmake -S . -B %BUILD_DIR% -G "MinGW Makefiles"
)

REM 3. Build the project using MinGW
echo Building PlanetsProject with mingw32-make...
cmake --build %BUILD_DIR% --config Debug --target PlanetsProject -j 12
exit
