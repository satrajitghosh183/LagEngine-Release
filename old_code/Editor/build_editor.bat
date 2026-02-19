@echo off
echo ========================================
echo   VerletX Engine Editor Build Script
echo ========================================
echo.

REM Check if build directory exists
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

echo.
echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..

if %ERRORLEVEL% neq 0 (
    echo.
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building editor...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Build completed successfully!
echo   Executable: build\bin\Release\VerletXEditor.exe
echo ========================================
echo.

pause

