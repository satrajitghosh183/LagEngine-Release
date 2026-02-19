#!/bin/bash

echo "========================================"
echo "  VerletX Engine Editor Build Script"
echo "========================================"
echo

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

echo
echo "Configuring with CMake..."
cmake ..

if [ $? -ne 0 ]; then
    echo
    echo "CMake configuration failed!"
    exit 1
fi

echo
echo "Building editor..."
cmake --build . --config Release -j$(nproc)

if [ $? -ne 0 ]; then
    echo
    echo "Build failed!"
    exit 1
fi

echo
echo "========================================"
echo "  Build completed successfully!"
echo "  Executable: build/bin/VerletXEditor"
echo "========================================"
echo

