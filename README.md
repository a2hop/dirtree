# dirtree

A cross-platform command-line utility for displaying directory structures in a textual tree format. Particularly useful for communicating folder structures to Large Language Models (LLMs).

## Features

- Displays directory trees in a hierarchical, visual format
- Cross-platform support (Linux, macOS, Windows)
- Customizable depth for directory traversal
- Automatic skipping of common system and temporary directories
- Cycle detection to prevent infinite recursion with symbolic links
- Can be built as a standalone executable or shared library

## Usage

```
dirtree [options] [directory]
```

### Options

- `-h, --help`: Display help message and exit
- `-d, --depth=LEVEL`: Maximum depth to display (default: no limit)
- `-a, --all`: Disable skipping of common directories/files

### Arguments

- `directory`: Directory to display (default: current directory)

### Examples

```bash
# Show tree for current directory
dirtree

# Show tree for specified directory
dirtree /path/to/dir

# Show tree with maximum depth of 2
dirtree -d 2 /path/to/dir

# Show tree for current directory with depth 3
dirtree --depth=3

# Show all files including those normally skipped
dirtree -a
```

### Skip Lists

By default, dirtree skips certain common directories and files that are typically not relevant for project structure visualization:

#### Skipped Directories
- `node_modules`, `.git`, `.vscode`, `__pycache__`, `venv`, `.idea`
- Windows-specific: `$RECYCLE.BIN`, `System Volume Information`, `Windows.old`, `AppData`, `Temp`

#### Skipped Files
- `.gitignore`, `.DS_Store`, `Thumbs.db`, `.env`
- Windows-specific: `desktop.ini`, `ntuser.dat`, etc.

Use the `-a` flag to display all directories and files without skipping.

## Building from Source

### Prerequisites

#### For Linux/macOS

- GCC or compatible C compiler
- Make

#### For Cross-compiling to Windows from Linux

- MinGW-w64 toolchain
- Install on Debian/Ubuntu with:
  ```bash
  sudo apt-get install mingw-w64
  ```

### Build Instructions

#### For Linux/macOS

```bash
# Clone the repository
git clone https://github.com/yourusername/dirtree.git
cd dirtree

# Build the executable
make

# Run the program
./dirtree
```

#### Cross-compile for Windows from Linux

```bash
# Build for Windows
./build.sh

# This creates dirtree.exe which can be used on Windows
```

### Manual Compilation

Linux/macOS:
```bash
gcc -Wall -Wextra -O2 -o dirtree dirtree.c
```

Windows (using MinGW):
```bash
x86_64-w64-mingw32-gcc -o dirtree.exe dirtree.c -static
```

## License

This project is open source and available under the MIT License.
