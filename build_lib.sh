#!/bin/bash

# Clean up previous build files
echo "Cleaning up previous library files..."
make clean
rm -f libdirtree.so libdirtree.dll

# Build the shared library for Linux
echo "Building Linux shared library..."
make shared

# Cross-compile shared library for Windows
echo "Building Windows shared library..."
x86_64-w64-mingw32-gcc -shared -o libdirtree.dll dirtree.c

echo "Library build completed successfully."
echo "Libraries created: libdirtree.so, libdirtree.dll"
