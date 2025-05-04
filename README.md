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

## Installation

### Downloading Pre-built Binaries

You can download the latest pre-built binaries from the [GitHub Releases](https://github.com/a2hop/dirtree/releases) page:

- **Linux**: Download the `dirtree-linux.zip` file, extract it, and make it executable with `chmod +x dirtree`
- **Windows**: Download the `dirtree-windows.zip` file and extract the `dirtree.exe` executable

### Building from Source

#### Prerequisites

##### For Linux/macOS

- GCC or compatible C compiler
- Make

##### For Cross-compiling to Windows from Linux

- MinGW-w64 toolchain
- Install on Debian/Ubuntu with:
  ```bash
  sudo apt-get install mingw-w64
  ```

#### Build Instructions

##### Building the Executable

```bash
# Make the build script executable
chmod +x build.sh

# Run the build script
./build.sh
```

This creates:
- `dirtree` (Linux/macOS executable)
- `dirtree.exe` (Windows executable)

##### Building the Shared Library

```bash
# Make the library build script executable
chmod +x build_lib.sh

# Run the library build script
./build_lib.sh
```

This creates:
- `libdirtree.so` (Linux/macOS shared library)
- `libdirtree.dll` (Windows shared library)

#### Manual Compilation

##### Executable

Linux/macOS:
```bash
gcc -Wall -Wextra -O2 -o dirtree dirtree.c
```

Windows (using MinGW):
```bash
x86_64-w64-mingw32-gcc -o dirtree.exe dirtree.c -static
```

##### Shared Library

Linux/macOS:
```bash
gcc -Wall -Wextra -O2 -shared -fPIC -o libdirtree.so dirtree.c
```

Windows (using MinGW):
```bash
x86_64-w64-mingw32-gcc -shared -o libdirtree.dll dirtree.c
```

## Continuous Integration

This project uses GitHub Actions for continuous integration:

- Automatically builds Linux and Windows executables on each push to the main branch
- When a new release is created, automatically attaches the built executables to the release

## License

This project is open source and available under the BSD 3-Clause License.
