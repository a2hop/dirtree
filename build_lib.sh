#!/bin/bash

# Clean up previous build files
echo "Cleaning up previous library files..."
make clean
rm -f libdirtree.dll

# Build the shared library for Linux
echo "Building Linux shared library..."
make shared

# Build the executable
echo "Building executable..."
make

# Cross-compile shared library for Windows
echo "Building Windows shared library..."
x86_64-w64-mingw32-gcc -Wall -std=c99 -D_WIN32 -DDIRTREE_LIBRARY_ONLY -shared \
    -o libdirtree.dll dirtree.c

echo "Library build completed successfully."
echo "Files created:"
echo "  - libdirtree.so.1.0.0 (Linux shared library)"
echo "  - libdirtree.so.1 (symlink)"
echo "  - libdirtree.so (symlink)"
echo "  - libdirtree.dll (Windows shared library)"
echo "  - dirtree (executable)"
