#!/bin/bash

# Clean up previous build files
echo "Cleaning up previous build files..."
make clean
rm -f dirtree.exe

# Build the executable
echo "Building Linux executable..."
make

# Cross-compile for Windows
echo "Building Windows executable..."
x86_64-w64-mingw32-gcc -o dirtree.exe dirtree.c -static

echo "Build completed successfully."
echo "Binaries created: dirtree, dirtree.exe"

