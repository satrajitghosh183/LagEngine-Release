@echo off
echo ============================================
echo  GameEngine - Ollama Setup Script
echo  Installs Ollama with models on D: drive
echo ============================================
echo.

:: Set Ollama to store models on D: drive
set OLLAMA_MODELS=D:\OllamaModels
setx OLLAMA_MODELS "D:\OllamaModels" >nul 2>&1

echo [1/4] Set OLLAMA_MODELS=D:\OllamaModels
echo.

:: Create directory
if not exist "D:\OllamaModels" mkdir "D:\OllamaModels"
echo [2/4] Created D:\OllamaModels directory
echo.

:: Check if Ollama is already installed
where ollama >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [3/4] Ollama is already installed
    ollama --version
) else (
    echo [3/4] Ollama not found. Please install from:
    echo       https://ollama.com/download/windows
    echo.
    echo After installing, re-run this script to pull models.
    echo.
    pause
    exit /b
)

echo.
echo [4/4] Pulling recommended models for shader generation...
echo.

echo Pulling codellama:13b (recommended for GLSL)...
ollama pull codellama:13b

echo.
echo Pulling deepseek-coder:6.7b (lighter alternative)...
ollama pull deepseek-coder:6.7b

echo.
echo ============================================
echo  Setup Complete!
echo ============================================
echo.
echo Models are stored in: D:\OllamaModels
echo.
echo To start Ollama server: ollama serve
echo To verify: ollama list
echo.
echo The Shader Assistant panel in the editor will
echo auto-detect the running Ollama server.
echo.
pause
