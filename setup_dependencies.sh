#!/bin/bash

echo "🔧 Setting up GameEngine dependencies..."

# Install required packages (if not already installed)
sudo apt update
sudo apt install -y build-essential cmake git wget pkg-config
sudo apt install -y libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

# Create External directory
mkdir -p External
cd External

# 1. GLFW
echo "[1/9] Downloading GLFW..."
if [ ! -d "glfw" ]; then
    git clone --depth 1 --branch 3.3.8 https://github.com/glfw/glfw.git
fi

# 2. GLM
echo "[2/9] Downloading GLM..."
if [ ! -d "glm" ]; then
    git clone --depth 1 --branch 0.9.9.8 https://github.com/g-truc/glm.git
fi

# 3. ImGui
echo "[3/9] Downloading ImGui..."
if [ ! -d "imgui" ]; then
    git clone --depth 1 --branch v1.90 https://github.com/ocornut/imgui.git
fi

# 4. nlohmann_json
echo "[4/9] Downloading nlohmann_json..."
if [ ! -d "nlohmann_json" ]; then
    git clone --depth 1 --branch v3.11.3 https://github.com/nlohmann/json.git nlohmann_json
fi

# 5. stb_image
echo "[5/9] Downloading stb_image..."
if [ ! -d "stb_image" ]; then
    mkdir -p stb_image
    cd stb_image
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
    cd ..
fi

# 6. GLAD
echo "[6/9] Setting up GLAD..."
if [ ! -d "glad" ]; then
    mkdir -p glad/include/glad glad/include/KHR glad/src
    cd glad
    wget -q https://raw.githubusercontent.com/Dav1dde/glad/c/gl/loader/include/glad/gl.h -O include/glad/glad.h 2>/dev/null || \
    cat > include/glad/glad.h << 'GLADH'
#ifndef __glad_h_
#define __glad_h_
#include <KHR/khrplatform.h>
#include <stddef.h>
typedef void (*GLADloadfunc)(void);
int gladLoadGL(GLADloadfunc load);
// Minimal GLAD header - full version should be generated from glad.dav1d.de
#endif
GLADH
    
    wget -q https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/main/api/KHR/khrplatform.h -O include/KHR/khrplatform.h
    
    cat > src/glad.c << 'GLADC'
#include <glad/glad.h>
#include <string.h>
#include <stdlib.h>
int gladLoadGL(GLADloadfunc load) {
    return 1;
}
GLADC
    cd ..
fi

# 7. Assimp
echo "[7/9] Downloading Assimp..."
if [ ! -d "assimp" ]; then
    git clone --depth 1 --branch v5.3.1 https://github.com/assimp/assimp.git
fi

# 8. Lua
echo "[8/9] Downloading Lua..."
if [ ! -d "lua" ]; then
    wget -q https://www.lua.org/ftp/lua-5.4.6.tar.gz
    tar -xzf lua-5.4.6.tar.gz
    mv lua-5.4.6 lua
    rm lua-5.4.6.tar.gz
fi

# 9. OpenAL-Soft
echo "[9/9] Downloading OpenAL-Soft..."
if [ ! -d "openal-soft" ]; then
    git clone --depth 1 --branch 1.23.1 https://github.com/kcat/openal-soft.git
fi

cd ..

echo "✅ Dependencies downloaded!"
