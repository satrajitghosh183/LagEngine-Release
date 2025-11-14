#!/bin/bash

echo "================================="
echo "  Building GameEngine (WSL)"
echo "================================="

# Check if dependencies are set up
if [ ! -f "External/CMakeLists.txt" ]; then
    echo "Setting up dependencies..."
    bash setup_dependencies.sh
fi

# Clean build directory if it exists
if [ -d "build" ]; then
    echo "Cleaning previous build..."
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DBUILD_EXAMPLES=ON \
         -DBUILD_TESTS=OFF \
         -DCMAKE_C_COMPILER=gcc \
         -DCMAKE_CXX_COMPILER=g++

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build
echo "Building..."
cmake --build . -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "Build complete!"
echo "Binaries are in: build/bin/"
echo ""
echo "Run examples:"
echo "  ./build/bin/HelloTriangle"
echo "  ./build/bin/PhysicsDemo"
echo "  ./build/bin/ClothSimulation"
echo "  ./build/bin/CharacterController"
echo "  ./build/bin/FluidSimulation"
echo "  ./build/bin/RobotArm"
echo ""

