#!/bin/bash
# Clean and rebuild script

echo "Cleaning build directory..."
cd build
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile

echo "Cleaning parent directory CMake files..."
cd ..
rm -f CMakeCache.txt cmake_install.cmake
rm -rf CMakeFiles

echo "Reconfiguring CMake from build directory..."
cd build
cmake ..

echo "Building..."
make

echo "Done! Run ./featherGL to execute."

