@echo off
REM ═══════════════════════════════════════════════════════════════════════════
REM  LAGEngine — Windows launcher for setup.sh
REM  This script finds Git Bash and runs setup.sh inside it.
REM ═══════════════════════════════════════════════════════════════════════════

setlocal

REM Try common Git Bash locations
set "BASH_EXE="
if exist "C:\Program Files\Git\bin\bash.exe" (
    set "BASH_EXE=C:\Program Files\Git\bin\bash.exe"
) else if exist "C:\Program Files (x86)\Git\bin\bash.exe" (
    set "BASH_EXE=C:\Program Files (x86)\Git\bin\bash.exe"
) else (
    where bash >nul 2>&1
    if %errorlevel% equ 0 (
        set "BASH_EXE=bash"
    )
)

if "%BASH_EXE%"=="" (
    echo.
    echo  ERROR: Git Bash not found.
    echo.
    echo  Install Git for Windows from https://git-scm.com/download/win
    echo  Or run this script from WSL:  bash setup.sh
    echo.
    pause
    exit /b 1
)

echo  Launching setup.sh in Git Bash...
"%BASH_EXE%" "%~dp0setup.sh" %*
