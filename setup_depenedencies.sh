#!/bin/bash

echo "🔧 GameEngine Dependencies Setup"
echo "================================"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Create External directory
mkdir -p External
cd External

echo -e "${BLUE}📥 Downloading dependencies...${NC}"

# 1. GLFW (Windowing)
echo -e "${GREEN}[1/9] GLFW${NC}"
if [ ! -d "glfw" ]; then
    git clone --depth 1 --branch 3.3.8 https://github.com/glfw/glfw.git
    echo "✓ GLFW downloaded"
else
    echo "✓ GLFW already exists"
fi

# 2. GLM (Math library)
echo -e "${GREEN}[2/9] GLM${NC}"
if [ ! -d "glm" ]; then
    git clone --depth 1 --branch 0.9.9.8 https://github.com/g-truc/glm.git
    echo "✓ GLM downloaded"
else
    echo "✓ GLM already exists"
fi

# 3. ImGui (UI)
echo -e "${GREEN}[3/9] ImGui${NC}"
if [ ! -d "imgui" ]; then
    git clone --depth 1 --branch v1.90 https://github.com/ocornut/imgui.git
    echo "✓ ImGui downloaded"
else
    echo "✓ ImGui already exists"
fi

# 4. nlohmann_json (JSON parsing)
echo -e "${GREEN}[4/9] nlohmann_json${NC}"
if [ ! -d "nlohmann_json" ]; then
    git clone --depth 1 --branch v3.11.3 https://github.com/nlohmann/json.git nlohmann_json
    echo "✓ nlohmann_json downloaded"
else
    echo "✓ nlohmann_json already exists"
fi

# 5. stb (Image loading)
echo -e "${GREEN}[5/9] stb_image${NC}"
if [ ! -d "stb_image" ]; then
    mkdir -p stb_image
    cd stb_image
    wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
    cd ..
    echo "✓ stb_image downloaded"
else
    echo "✓ stb_image already exists"
fi

# 6. GLAD (OpenGL loader)
echo -e "${GREEN}[6/9] GLAD${NC}"
if [ ! -d "glad" ]; then
    echo "Generating GLAD files..."
    # Download pre-generated GLAD for OpenGL 4.5 Core
    mkdir -p glad/include/glad glad/include/KHR glad/src
    
    # Download glad.h
    wget -O glad/include/glad/glad.h "https://raw.githubusercontent.com/Dav1dde/glad/master/include/glad/glad.h"
    
    # Download khrplatform.h
    wget -O glad/include/KHR/khrplatform.h "https://raw.githubusercontent.com/Dav1dde/glad/master/include/KHR/khrplatform.h"
    
    # Download glad.c
    wget -O glad/src/glad.c "https://raw.githubusercontent.com/Dav1dde/glad/master/src/glad.c"
    
    echo "✓ GLAD downloaded"
else
    echo "✓ GLAD already exists"
fi

# 7. Assimp (Model loading)
echo -e "${GREEN}[7/9] Assimp${NC}"
if [ ! -d "assimp" ]; then
    git clone --depth 1 --branch v5.3.1 https://github.com/assimp/assimp.git
    echo "✓ Assimp downloaded"
else
    echo "✓ Assimp already exists"
fi

# 8. Lua (Scripting)
echo -e "${GREEN}[8/9] Lua${NC}"
if [ ! -d "lua" ]; then
    wget https://www.lua.org/ftp/lua-5.4.6.tar.gz
    tar -xzf lua-5.4.6.tar.gz
    mv lua-5.4.6 lua
    rm lua-5.4.6.tar.gz
    echo "✓ Lua downloaded"
else
    echo "✓ Lua already exists"
fi

# 9. OpenAL Soft (Audio)
echo -e "${GREEN}[9/9] OpenAL-Soft${NC}"
if [ ! -d "openal-soft" ]; then
    git clone --depth 1 --branch 1.23.1 https://github.com/kcat/openal-soft.git
    echo "✓ OpenAL-Soft downloaded"
else
    echo "✓ OpenAL-Soft already exists"
fi

cd ..

echo -e "${BLUE}📝 Creating CMakeLists.txt for external libraries...${NC}"

# Create External/CMakeLists.txt
cat > External/CMakeLists.txt << 'EOF'
# External Dependencies

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# GLM (header-only)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# ImGui
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    imgui
    imgui/backends
)
target_link_libraries(imgui PUBLIC glfw)

# nlohmann_json (header-only)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE nlohmann_json/include)

# stb_image
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE stb_image)

# GLAD
add_library(glad STATIC glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include)

# Assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(assimp)

# Lua
add_library(lua STATIC
    lua/src/lapi.c
    lua/src/lauxlib.c
    lua/src/lbaselib.c
    lua/src/lcode.c
    lua/src/lcorolib.c
    lua/src/lctype.c
    lua/src/ldblib.c
    lua/src/ldebug.c
    lua/src/ldo.c
    lua/src/ldump.c
    lua/src/lfunc.c
    lua/src/lgc.c
    lua/src/linit.c
    lua/src/liolib.c
    lua/src/llex.c
    lua/src/lmathlib.c
    lua/src/lmem.c
    lua/src/loadlib.c
    lua/src/lobject.c
    lua/src/lopcodes.c
    lua/src/loslib.c
    lua/src/lparser.c
    lua/src/lstate.c
    lua/src/lstring.c
    lua/src/lstrlib.c
    lua/src/ltable.c
    lua/src/ltablib.c
    lua/src/ltm.c
    lua/src/lundump.c
    lua/src/lutf8lib.c
    lua/src/lvm.c
    lua/src/lzio.c
)
target_include_directories(lua PUBLIC lua/src)
if(UNIX)
    target_compile_definitions(lua PRIVATE LUA_USE_LINUX)
    target_link_libraries(lua PUBLIC m dl)
endif()

# OpenAL Soft
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
add_subdirectory(openal-soft)
EOF

echo -e "${GREEN}✅ All dependencies downloaded successfully!${NC}"
echo ""
echo "Next steps:"
echo "  1. cd .."
echo "  2. mkdir build && cd build"
echo "  3. cmake .."
echo "  4. cmake --build ."