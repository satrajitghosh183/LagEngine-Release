#!/bin/bash
# Cross-platform build script for LAG Engine with dependency installation
# Supports Windows (MinGW/MSYS2), Linux, WSL, and macOS

# Enable command echo and exit on error
set -e

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "➡️  Setting up LAG Engine from: $SCRIPT_DIR"
cd "$SCRIPT_DIR" || { echo "❌ Failed to change directory to script location"; exit 1; }

# Detect OS and system configuration
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    OS_TYPE="Windows"
    # For Windows, determine if using MinGW/MSYS2
    if command -v mingw32-make &> /dev/null; then
        MAKE_CMD="mingw32-make"
    else
        MAKE_CMD="make"
    fi
    # Default to 4 threads if detection fails
    NUM_CORES=$(nproc 2>/dev/null || echo 4)
elif [[ "$(uname -r)" == *Microsoft* || "$(uname -r)" == *microsoft* ]]; then
    OS_TYPE="WSL"
    MAKE_CMD="make"
    NUM_CORES=$(nproc 2>/dev/null || echo 4)
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS_TYPE="macOS"
    MAKE_CMD="make"
    NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
else
    OS_TYPE="Linux"
    MAKE_CMD="make"
    NUM_CORES=$(nproc 2>/dev/null || echo 4)
fi

echo "🖥️  Detected OS: $OS_TYPE with $NUM_CORES cores"

# Function to check if a command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Function to install dependencies based on OS
install_dependencies() {
    echo "📦 Installing dependencies..."
    
    case $OS_TYPE in
        Windows)
            # For MSYS2/MinGW
            if command_exists pacman; then
                echo "🔄 Updating package database..."
                pacman -Syu --noconfirm
                echo "📥 Installing dependencies using pacman..."
                pacman -S --noconfirm \
                    mingw-w64-x86_64-gcc \
                    mingw-w64-x86_64-cmake \
                    mingw-w64-x86_64-make \
                    mingw-w64-x86_64-glfw \
                    mingw-w64-x86_64-glm \
                    mingw-w64-x86_64-sfml \
                    mingw-w64-x86_64-opengl-headers \
                    git
            else
                echo "⚠️  MSYS2/MinGW package manager not found."
                echo "⚠️  Please install the following dependencies manually:"
                echo "    - GCC/MinGW"
                echo "    - CMake"
                echo "    - GLFW"
                echo "    - GLM"
                echo "    - SFML 2.5+"
                echo "    - OpenGL development headers"
                echo "    - Git"
                echo "⚠️  Continuing without installing dependencies..."
            fi
            ;;
            
        Linux|WSL)
            # For Debian/Ubuntu-based systems
            if command_exists apt-get; then
                echo "🔄 Updating package database..."
                sudo apt-get update
                echo "📥 Installing dependencies using apt..."
                sudo apt-get install -y \
                    build-essential \
                    cmake \
                    libglfw3-dev \
                    libglm-dev \
                    libsfml-dev \
                    mesa-common-dev \
                    libgl1-mesa-dev \
                    libglu1-mesa-dev \
                    git
            # For Red Hat/Fedora-based systems
            elif command_exists dnf; then
                echo "🔄 Updating package database..."
                sudo dnf check-update || true
                echo "📥 Installing dependencies using dnf..."
                sudo dnf install -y \
                    gcc-c++ \
                    cmake \
                    glfw-devel \
                    glm-devel \
                    SFML-devel \
                    mesa-libGL-devel \
                    mesa-libGLU-devel \
                    git
            # For Arch-based systems
            elif command_exists pacman; then
                echo "🔄 Updating package database..."
                sudo pacman -Syu --noconfirm
                echo "📥 Installing dependencies using pacman..."
                sudo pacman -S --noconfirm \
                    gcc \
                    cmake \
                    glfw-x11 \
                    glm \
                    sfml \
                    mesa \
                    git
            else
                echo "⚠️  No supported package manager found."
                echo "⚠️  Please install the following dependencies manually:"
                echo "    - GCC/C++ compiler"
                echo "    - CMake"
                echo "    - GLFW"
                echo "    - GLM"
                echo "    - SFML 2.5+"
                echo "    - OpenGL development libraries"
                echo "    - Git"
                echo "⚠️  Continuing without installing dependencies..."
            fi
            ;;
            
        macOS)
            # For macOS with Homebrew
            if command_exists brew; then
                echo "🔄 Updating Homebrew..."
                brew update
                echo "📥 Installing dependencies using Homebrew..."
                brew install \
                    cmake \
                    glfw \
                    glm \
                    sfml \
                    git
            else
                echo "⚠️  Homebrew not found."
                echo "⚠️  Please install Homebrew first: https://brew.sh/"
                echo "⚠️  Then install the following dependencies:"
                echo "    - CMake"
                echo "    - GLFW"
                echo "    - GLM"
                echo "    - SFML 2.5+"
                echo "    - Git"
                echo "⚠️  Continuing without installing dependencies..."
            fi
            ;;
    esac
    
    echo "✅ Dependency installation completed!"
}

# Function to download and setup external libraries not available via package managers
setup_external_libs() {
    echo "📚 Setting up additional libraries..."
    
    # Create external libs directory if it doesn't exist
    mkdir -p external
    cd external
    
    # Download GLAD if not already present
    if [ ! -d "glad" ]; then
        echo "📥 Downloading GLAD (OpenGL loader)..."
        git clone --depth 1 https://github.com/Dav1dde/glad.git
        cd glad
        python -m pip install -r requirements.txt || echo "⚠️ Python pip failed, please install GLAD manually"
        python -m glad --profile=core --api=gl=3.3 --generator=c --out-path=.
        cd ..
    fi
    
    # Download stb_image if not already present
    if [ ! -d "stb" ]; then
        echo "📥 Downloading stb_image..."
        git clone --depth 1 https://github.com/nothings/stb.git
    fi
    
    # Download Dear ImGui if not already present
    if [ ! -d "imgui" ]; then
        echo "📥 Downloading Dear ImGui..."
        git clone --depth 1 https://github.com/ocornut/imgui.git
    fi
    
    # Download ImGui-SFML if not already present
    if [ ! -d "imgui-sfml" ]; then
        echo "📥 Downloading ImGui-SFML..."
        git clone --depth 1 https://github.com/eliasdaler/imgui-sfml.git
    fi
    
    cd ..
    echo "✅ External libraries setup completed!"
}

# Ask for confirmation to install dependencies
read -p "🔍 Do you want to install dependencies? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    install_dependencies
fi

# Ask for confirmation to setup external libraries
read -p "🔍 Do you want to setup external libraries (GLAD, stb_image, ImGui)? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    setup_external_libs
fi

# Create build directory
echo "🧹 Cleaning build directory..."
rm -rf build
mkdir -p build
cd build || { echo "❌ Failed to change to build directory"; exit 1; }

# Run CMake
echo "🔧 Running CMake..."
cmake .. || { echo "❌ CMake failed"; exit 1; }

# Build using appropriate method for the platform
echo "🔨 Building with $NUM_CORES threads..."
if [[ "$OS_TYPE" == "Windows" ]]; then
    # On Windows, use CMake's build command or MinGW make
    if command -v cmake --build &> /dev/null; then
        cmake --build . --config Release -- -j $NUM_CORES || { echo "❌ Build failed"; exit 1; }
    else
        $MAKE_CMD -j $NUM_CORES || { echo "❌ Build failed"; exit 1; }
    fi
else
    # Linux, macOS, or WSL
    $MAKE_CMD -j $NUM_CORES || { echo "❌ Build failed"; exit 1; }
fi

echo "✅ Build completed successfully!"
echo ""

# Determine executable extension
EXT=""
if [[ "$OS_TYPE" == "Windows" ]]; then
    EXT=".exe"
fi

# Show run instructions
echo "To run the demos, use:"
echo "  ./build/main2D$EXT - For 2D cloth and cannon simulation"
echo "  ./build/main3D$EXT - For 3D cloth and ball physics"

# Add proper permissions for Linux/WSL/macOS
if [[ "$OS_TYPE" != "Windows" ]]; then
    chmod +x ./main2D ./main3D 2>/dev/null
    echo "✅ Executable permissions set"
fi

echo "🚀 Happy physics simulating!"