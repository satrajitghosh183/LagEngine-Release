#!/bin/bash

echo "================================="
echo "  Building GameEngine"
echo "================================="

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DBUILD_EDITOR=ON \
         -DBUILD_RUNTIME=ON \
         -DBUILD_EXAMPLES=ON

# Build
cmake --build . --config Release -j$(nproc)

echo ""
echo "Build complete!"
echo "Binaries are in: build/bin/"
echo ""
echo "Run example:"
echo "  ./build/bin/PhysicsDemo"