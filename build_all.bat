@echo off
echo =================================
echo   Building GameEngine
echo =================================

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
cmake .. -G "Visual Studio 17 2022" ^
         -DCMAKE_BUILD_TYPE=Release ^
         -DBUILD_EDITOR=ON ^
         -DBUILD_RUNTIME=ON ^
         -DBUILD_EXAMPLES=ON

REM Build
cmake --build . --config Release

echo.
echo Build complete!
echo Binaries are in: build\bin\Release\
echo.
echo Run example:
echo   build\bin\Release\PhysicsDemo.exe

pause