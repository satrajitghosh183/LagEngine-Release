@echo off
REM Quick start script for CUDA GL Demo RL (Windows)

echo ==========================================
echo CUDA GL Demo - RL Quick Start
echo ==========================================

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python not found. Please install Python 3.8+
    exit /b 1
)

REM Install dependencies
echo.
echo Installing Python dependencies...
pip install -r requirements.txt

REM Check if executable exists
set EXECUTABLE=..\build\cuda_gl_demo.exe
if not exist "%EXECUTABLE%" (
    echo.
    echo Warning: Executable not found at %EXECUTABLE%
    echo Please build the project first:
    echo   cd ..\build
    echo   cmake ..
    echo   cmake --build .
    echo.
    set /p CONTINUE="Continue anyway? (y/n) "
    if /i not "%CONTINUE%"=="y" exit /b 1
    set EXECUTABLE=
)

REM Run a quick test
echo.
echo Running quick test...
python example_usage.py basic

echo.
echo ==========================================
echo Quick start complete!
echo ==========================================
echo.
echo Next steps:
echo   1. Collect data:    python collect_data.py
echo   2. Train model:     python train_rl.py --timesteps 500
echo   3. Evaluate model:  python evaluate_rl.py rl_models\cuda_gl_rl_final.zip
echo.
echo For more information, see README.md
echo.

pause

