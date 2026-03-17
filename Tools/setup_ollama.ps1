# GameEngine - Ollama Setup Script (PowerShell)
# Installs Ollama with models on D: drive

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " GameEngine - Ollama Setup Script" -ForegroundColor Cyan
Write-Host " Installs Ollama with models on D: drive" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Set environment variable for D: drive storage
Write-Host "[1/5] Setting OLLAMA_MODELS=D:\OllamaModels..." -ForegroundColor Yellow
$env:OLLAMA_MODELS = "D:\OllamaModels"
[System.Environment]::SetEnvironmentVariable("OLLAMA_MODELS", "D:\OllamaModels", "User")
Write-Host "       Environment variable set!" -ForegroundColor Green
Write-Host ""

# Step 2: Create directory
Write-Host "[2/5] Creating D:\OllamaModels directory..." -ForegroundColor Yellow
if (-not (Test-Path "D:\OllamaModels")) {
    New-Item -ItemType Directory -Path "D:\OllamaModels" -Force | Out-Null
}
Write-Host "       Directory ready!" -ForegroundColor Green
Write-Host ""

# Step 3: Install Ollama if not present
Write-Host "[3/5] Checking Ollama installation..." -ForegroundColor Yellow
$ollamaPath = Get-Command ollama -ErrorAction SilentlyContinue

if (-not $ollamaPath) {
    Write-Host "       Ollama not found. Installing..." -ForegroundColor Yellow
    Write-Host ""
    
    # Install Ollama using the official PowerShell script
    try {
        Invoke-RestMethod https://ollama.com/install.ps1 | Invoke-Expression
        Write-Host ""
        Write-Host "       Ollama installed successfully!" -ForegroundColor Green
        
        # Refresh PATH
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    }
    catch {
        Write-Host "       Failed to install Ollama: $_" -ForegroundColor Red
        Write-Host "       Please install manually from: https://ollama.com/download/windows" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
} else {
    Write-Host "       Ollama is already installed!" -ForegroundColor Green
    & ollama --version
}
Write-Host ""

# Step 4: Start Ollama server in background if not running
Write-Host "[4/5] Ensuring Ollama server is running..." -ForegroundColor Yellow
$ollamaProcess = Get-Process -Name "ollama" -ErrorAction SilentlyContinue
if (-not $ollamaProcess) {
    Write-Host "       Starting Ollama server..." -ForegroundColor Yellow
    Start-Process -FilePath "ollama" -ArgumentList "serve" -WindowStyle Hidden
    Start-Sleep -Seconds 3
    Write-Host "       Ollama server started!" -ForegroundColor Green
} else {
    Write-Host "       Ollama server already running!" -ForegroundColor Green
}
Write-Host ""

# Step 5: Pull recommended models
Write-Host "[5/5] Pulling recommended models..." -ForegroundColor Yellow
Write-Host ""

Write-Host "       Pulling codellama:13b (for CMake & code generation)..." -ForegroundColor Cyan
& ollama pull codellama:13b

Write-Host ""
Write-Host "       Pulling deepseek-coder:6.7b (for shader generation)..." -ForegroundColor Cyan
& ollama pull deepseek-coder:6.7b

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host " Setup Complete!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "Models stored in: D:\OllamaModels" -ForegroundColor White
Write-Host ""
Write-Host "Available in editor:" -ForegroundColor White
Write-Host "  - AI Assistant panel (code help)" -ForegroundColor Gray
Write-Host "  - Build panel (CMake generation)" -ForegroundColor Gray
Write-Host "  - Shader Assistant (GLSL generation)" -ForegroundColor Gray
Write-Host ""
Write-Host "To verify: ollama list" -ForegroundColor Yellow
Write-Host ""

& ollama list

Write-Host ""
Read-Host "Press Enter to close"
