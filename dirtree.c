
/*
 * Copyright (c) 2025 Lucas Kafarski
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions of the BSD 3-Clause
 * License are met.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <limits.h>

// Windows-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define F_OK 0
    #define access _access
    // Don't redefine PATH_MAX as it's already defined in Windows headers
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

// Tree drawing characters for different platforms
#ifdef _WIN32
    // ASCII characters for Windows
    #define TREE_BRANCH "|-- "
    #define TREE_CORNER "+-- "
    #define TREE_VERTICAL "|   "
    #define TREE_SPACE "    "
#else
    // Unicode characters for Unix-like systems
    #define TREE_BRANCH "├── "
    #define TREE_CORNER "└── "
    #define TREE_VERTICAL "│   "
    #define TREE_SPACE "    "
#endif

// Global flag to control skipping behavior
bool skip_enabled = true;

// Global skiplist - directories to skip
const char *skiplist[] = {
    // Cross-platform directories
    "node_modules",
    ".git",
    ".vscode",
    "__pycache__",
    "venv",
    ".idea",
    // Windows-specific directories
    "$RECYCLE.BIN",
    "System Volume Information",
    "Windows.old",
    "AppData",
    "Temp",
    NULL  // NULL terminator to mark the end of the array
};

// Global skipfiles - files to skip
const char *skipfiles[] = {
    // Cross-platform files
    ".gitignore",
    ".DS_Store",
    "Thumbs.db",
    ".env",
    // Windows-specific files
    "desktop.ini",
    "ntuser.dat",
    "NTUSER.DAT",
    "ntuser.dat.LOG1",
    "ntuser.dat.LOG2",
    "ntuser.ini",
    NULL  // NULL terminator to mark the end of the array
};

// Binary name to skip when listing the directory where the binary is located
char *program_binary_name = NULL;

// Check if a directory name is in the skiplist
bool is_in_skiplist(const char *name) {
    // If skipping is disabled, return false
    if (!skip_enabled) {
        return false;
    }
    
    for (int i = 0; skiplist[i] != NULL; i++) {
        if (strcmp(name, skiplist[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Check if a file name is in the skipfiles list
bool is_in_skipfiles(const char *name) {
    // If skipping is disabled, return false
    if (!skip_enabled) {
        return false;
    }
    
    for (int i = 0; skipfiles[i] != NULL; i++) {
        if (strcmp(name, skipfiles[i]) == 0) {
            return true;
        }
    }
    
    // Skip the program's own binary file if we're in its directory
    if (program_binary_name && strcmp(name, program_binary_name) == 0) {
        return true;
    }
    
    return false;
}

// Print help message
void print_help(const char *program_name) {
    printf("Directory Tree Utility\n");
    printf("\n");
    printf("Usage: %s [options] [directory]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help               Display this help message and exit\n");
    printf("  -d, --depth=LEVEL        Maximum depth to display (default: no limit)\n");
    printf("  -a, --all                Disable skipping of common directories/files\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  directory                Directory to display (default: current directory)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                       # Show tree for current directory\n", program_name);
    printf("  %s /path/to/dir          # Show tree for specified directory\n", program_name);
    printf("  %s -d 2 /path/to/dir     # Show tree with maximum depth of 2\n", program_name);
    printf("  %s --depth=3             # Show tree for current directory with depth 3\n", program_name);
    printf("  %s -a                    # Show all files including those normally skipped\n", program_name);
    printf("\n");
}

// Structure to represent a hash table for visited directories
typedef struct {
    char **paths;
    int size;
    int capacity;
} VisitedDirs;

// Initialize the visited directories hash table
void init_visited_dirs(VisitedDirs *visited, int capacity) {
    visited->paths = (char **)malloc(capacity * sizeof(char *));
    if (!visited->paths) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    visited->size = 0;
    visited->capacity = capacity;
    for (int i = 0; i < capacity; i++) {
        visited->paths[i] = NULL;
    }
}

// Check if a directory has been visited
bool is_visited(VisitedDirs *visited, const char *path) {
    for (int i = 0; i < visited->size; i++) {
        if (strcmp(visited->paths[i], path) == 0) {
            return true;
        }
    }
    return false;
}

// Mark a directory as visited
void mark_visited(VisitedDirs *visited, const char *path) {
    if (visited->size >= visited->capacity) {
        visited->capacity *= 2;
        visited->paths = (char **)realloc(visited->paths, visited->capacity * sizeof(char *));
        if (!visited->paths) {
            perror("Memory reallocation failed");
            exit(EXIT_FAILURE);
        }
    }
    
    visited->paths[visited->size] = strdup(path);
    if (!visited->paths[visited->size]) {
        perror("String duplication failed");
        exit(EXIT_FAILURE);
    }
    visited->size++;
}

// Free the visited directories hash table
void free_visited_dirs(VisitedDirs *visited) {
    for (int i = 0; i < visited->size; i++) {
        free(visited->paths[i]);
    }
    free(visited->paths);
}

// Structure to hold directory entry information
typedef struct {
    char *path;
    char *name;
} DirEntry;

// Compare function for qsort
int compare_entries(const void *a, const void *b) {
    return strcmp(((DirEntry *)a)->name, ((DirEntry *)b)->name);
}

// Get absolute path
char *get_absolute_path(const char *path) {
    char abs_path[PATH_MAX];
    
#ifdef _WIN32
    if (_fullpath(abs_path, path, PATH_MAX) == NULL) {
        perror("Error getting absolute path");
        return NULL;
    }
#else
    if (realpath(path, abs_path) == NULL) {
        perror("Error getting absolute path");
        return NULL;
    }
#endif
    
    return strdup(abs_path);
}

// Print the directory tree
void print_tree(const char *dir, const char *prefix, int max_depth, int current_depth, VisitedDirs *visited) {
    // Check if we've reached the maximum depth
    if (max_depth > 0 && current_depth > max_depth) {
        return;
    }
    
    // Skip the directory if it has already been visited
    if (is_visited(visited, dir)) {
        return;
    }
    mark_visited(visited, dir);
    
#ifdef _WIN32
    WIN32_FIND_DATA findData;
    char search_path[PATH_MAX];
    snprintf(search_path, PATH_MAX, "%s\\*", dir);
    
    HANDLE hFind = FindFirstFile(search_path, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    
    // Count and collect entries
    int count = 0;
    int capacity = 10;
    DirEntry *items = (DirEntry *)malloc(capacity * sizeof(DirEntry));
    if (!items) {
        perror("Memory allocation failed");
        FindClose(hFind);
        exit(EXIT_FAILURE);
    }
    
    do {
        // Skip . and ..
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
            continue;
        }
        
        // Skip hidden files if not in the root directory
        if (current_depth > 1 && findData.cFileName[0] == '.') {
            continue;
        }
        
        // Skip directories in the skiplist
        if (is_in_skiplist(findData.cFileName)) {
            continue;
        }
        
        // Create full path
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s\\%s", dir, findData.cFileName);
        
        // Check if it's a file that should be skipped
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && is_in_skipfiles(findData.cFileName)) {
            continue;
        }
        
        // Resize array if necessary
        if (count >= capacity) {
            capacity *= 2;
            items = (DirEntry *)realloc(items, capacity * sizeof(DirEntry));
            if (!items) {
                perror("Memory reallocation failed");
                FindClose(hFind);
                exit(EXIT_FAILURE);
            }
        }
        
        items[count].path = strdup(path);
        items[count].name = strdup(findData.cFileName);
        if (!items[count].path || !items[count].name) {
            perror("String duplication failed");
            FindClose(hFind);
            exit(EXIT_FAILURE);
        }
        
        count++;
    } while (FindNextFile(hFind, &findData));
    
    FindClose(hFind);
#else
    DIR *d = opendir(dir);
    if (!d) {
        return;
    }
    
    // Count and collect entries
    struct dirent *entry;
    int count = 0;
    int capacity = 10;
    DirEntry *items = (DirEntry *)malloc(capacity * sizeof(DirEntry));
    if (!items) {
        perror("Memory allocation failed");
        closedir(d);
        exit(EXIT_FAILURE);
    }
    
    while ((entry = readdir(d)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Skip hidden files if not in the root directory
        if (current_depth > 1 && entry->d_name[0] == '.') {
            continue;
        }
        
        // Skip directories in the skiplist
        if (is_in_skiplist(entry->d_name)) {
            continue;
        }
        
        // Create full path
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s/%s", dir, entry->d_name);
        
        // Check if it's a file that should be skipped
        struct stat st;
        if (stat(path, &st) == 0) {
            if (S_ISREG(st.st_mode) && is_in_skipfiles(entry->d_name)) {
                continue;
            }
        }
        
        // Resize array if necessary
        if (count >= capacity) {
            capacity *= 2;
            items = (DirEntry *)realloc(items, capacity * sizeof(DirEntry));
            if (!items) {
                perror("Memory reallocation failed");
                closedir(d);
                exit(EXIT_FAILURE);
            }
        }
        
        items[count].path = strdup(path);
        items[count].name = strdup(entry->d_name);
        if (!items[count].path || !items[count].name) {
            perror("String duplication failed");
            closedir(d);
            exit(EXIT_FAILURE);
        }
        
        count++;
    }
    
    closedir(d);
#endif
    
    // Sort entries alphabetically
    qsort(items, count, sizeof(DirEntry), compare_entries);
    
    // Process each item
    for (int i = 0; i < count; i++) {
        bool is_last = (i == count - 1);
        char current_prefix[PATH_MAX];
        char next_prefix[PATH_MAX];
        
        if (is_last) {
            snprintf(current_prefix, PATH_MAX, "%s%s", prefix, TREE_CORNER);
            snprintf(next_prefix, PATH_MAX, "%s%s", prefix, TREE_SPACE);
        } else {
            snprintf(current_prefix, PATH_MAX, "%s%s", prefix, TREE_BRANCH);
            snprintf(next_prefix, PATH_MAX, "%s%s", prefix, TREE_VERTICAL);
        }
        
        // Print the current item
        printf("%s%s\n", current_prefix, items[i].name);
        
#ifdef _WIN32
        // Check if it's a directory
        DWORD attrs = GetFileAttributes(items[i].path);
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            print_tree(items[i].path, next_prefix, max_depth, current_depth + 1, visited);
        }
#else
        // Check if it's a directory
        struct stat st;
        if (stat(items[i].path, &st) == 0 && S_ISDIR(st.st_mode)) {
            // Recursively process directories (but not symlinks)
            if (!S_ISLNK(st.st_mode)) {
                print_tree(items[i].path, next_prefix, max_depth, current_depth + 1, visited);
            }
        }
#endif
        
        free(items[i].path);
        free(items[i].name);
    }
    
    free(items);
}

int main(int argc, char *argv[]) {
    // Extract the program name from argv[0]
    char *program_path = strdup(argv[0]);
    if (program_path) {
        program_binary_name = strrchr(program_path, '/');
        if (program_binary_name) {
            program_binary_name++;  // Skip the '/'
        } else {
            program_binary_name = program_path;  // No '/' in path, use the whole string
        }
    }
    
    // Default values
    const char *dir = ".";
    int max_depth = -1;  // -1 means no limit
    
    // Define long options
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"depth", required_argument, 0, 'd'},
        {"all", no_argument, 0, 'a'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    // Parse options
    while ((c = getopt_long(argc, argv, "hd:a", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print_help(argv[0]);
                free(program_path);  // Free the allocated memory for program path
                return EXIT_SUCCESS;
            case 'd':
                max_depth = atoi(optarg);
                break;
            case 'a':
                skip_enabled = false;
                break;
            case '?':
                // getopt_long already printed an error message
                free(program_path);  // Free the allocated memory for program path
                return EXIT_FAILURE;
            default:
                abort();
        }
    }
    
    // Get directory from remaining arguments
    if (optind < argc) {
        dir = argv[optind];
    }
    
#ifdef _WIN32
    // Check if DIR exists and is a directory
    DWORD attrs = GetFileAttributes(dir);
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        fprintf(stderr, "Error: '%s' is not a directory or doesn't exist.\n", dir);
        free(program_path);
        return EXIT_FAILURE;
    }
#else
    // Ensure DIR exists and is a directory
    struct stat st;
    if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a directory or doesn't exist.\n", dir);
        free(program_path);  // Free the allocated memory for program path
        return EXIT_FAILURE;
    }
#endif
    
    // Convert to an absolute path
    char *abs_dir = get_absolute_path(dir);
    if (!abs_dir) {
        fprintf(stderr, "Error: Cannot access directory.\n");
        free(program_path);  // Free the allocated memory for program path
        return EXIT_FAILURE;
    }
    
    // Print the root directory name
    char *base_name = strrchr(abs_dir, '/');
    if (base_name == NULL) {
        base_name = abs_dir;
    } else {
        base_name++;  // Skip the '/'
    }
    printf("%s\n", base_name);
    
    // Initialize visited directories
    VisitedDirs visited;
    init_visited_dirs(&visited, 100);
    
    // Start printing the tree
    print_tree(abs_dir, "", max_depth, 1, &visited);
    
    // Clean up
    free_visited_dirs(&visited);
    free(abs_dir);
    free(program_path);  // Free the allocated memory for program path
    
    return EXIT_SUCCESS;
}
