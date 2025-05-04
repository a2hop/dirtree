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

// For strdup() and PATH_MAX
// Note: _GNU_SOURCE is already defined in the Makefile
#include <string.h>
#include <limits.h>
#ifdef __linux__
  #include <linux/limits.h>
#endif

// Include the public header
#include "dirtree.h"

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
    // Define PATH_MAX if not defined
    #ifndef PATH_MAX
        #define PATH_MAX 4096
    #endif
#endif

// Tree drawing characters for different formats
static const char *tree_chars[2][4] = {
    // ASCII characters
    {"|-- ", "+-- ", "|   ", "    "},
    // Unicode characters
    {"├── ", "└── ", "│   ", "    "}
};

#define TREE_BRANCH(fmt) (tree_chars[fmt][0])
#define TREE_CORNER(fmt) (tree_chars[fmt][1])
#define TREE_VERTICAL(fmt) (tree_chars[fmt][2])
#define TREE_SPACE(fmt) (tree_chars[fmt][3])

// Default skiplist - directories to skip
static const char *default_skiplist[] = {
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

// Default skipfiles - files to skip
static const char *default_skipfiles[] = {
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

// Library version
#define DIRTREE_VERSION "1.0.0"

// Structure to represent a hash table for visited directories
typedef struct {
    char **paths;
    int size;
    int capacity;
} VisitedDirs;

// Structure to hold directory entry information
typedef struct {
    char *path;
    char *name;
    bool is_dir;
} DirEntry;

// Compare function for qsort
static int compare_entries(const void *a, const void *b) {
    return strcmp(((DirEntry *)a)->name, ((DirEntry *)b)->name);
}

// Initialize the visited directories hash table
static void init_visited_dirs(VisitedDirs *visited, int capacity) {
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
static bool is_visited(VisitedDirs *visited, const char *path) {
    for (int i = 0; i < visited->size; i++) {
        if (strcmp(visited->paths[i], path) == 0) {
            return true;
        }
    }
    return false;
}

// Mark a directory as visited
static void mark_visited(VisitedDirs *visited, const char *path) {
    if (visited->size >= visited->capacity) {
        visited->capacity *= 2;
        visited->paths = (char **)realloc(visited->paths, visited->capacity * sizeof(char *));
        if (!visited->paths) {
            perror("Memory reallocation failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Use strdup with proper casting
    visited->paths[visited->size] = (char *)strdup(path);
    if (!visited->paths[visited->size]) {
        perror("String duplication failed");
        exit(EXIT_FAILURE);
    }
    visited->size++;
}

// Free the visited directories hash table
static void free_visited_dirs(VisitedDirs *visited) {
    for (int i = 0; i < visited->size; i++) {
        free(visited->paths[i]);
    }
    free(visited->paths);
}

// Get absolute path
static char *get_absolute_path(const char *path) {
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
    
    return (char *)strdup(abs_path);
}

// Check if a directory should be skipped
static bool should_skip_dir(const char *name, const DirtreeConfig *config) {
    // If skipping is disabled, return false
    if (!config->skip_common) {
        return false;
    }
    
    // Check default skiplist
    for (int i = 0; default_skiplist[i] != NULL; i++) {
        if (strcmp(name, default_skiplist[i]) == 0) {
            return true;
        }
    }
    
    // Check custom skiplist if provided
    if (config->custom_skip_dirs) {
        for (int i = 0; config->custom_skip_dirs[i] != NULL; i++) {
            if (strcmp(name, config->custom_skip_dirs[i]) == 0) {
                return true;
            }
        }
    }
    
    // Check for hidden files/dirs if skip_hidden is enabled
    if (config->skip_hidden && name[0] == '.') {
        return true;
    }
    
    return false;
}

// Check if a file should be skipped
static bool should_skip_file(const char *name, const DirtreeConfig *config) {
    // If skipping is disabled, return false
    if (!config->skip_common) {
        return false;
    }
    
    // Check default skipfiles
    for (int i = 0; default_skipfiles[i] != NULL; i++) {
        if (strcmp(name, default_skipfiles[i]) == 0) {
            return true;
        }
    }
    
    // Check custom skipfiles if provided
    if (config->custom_skip_files) {
        for (int i = 0; config->custom_skip_files[i] != NULL; i++) {
            if (strcmp(name, config->custom_skip_files[i]) == 0) {
                return true;
            }
        }
    }
    
    // Check for hidden files if skip_hidden is enabled
    if (config->skip_hidden && name[0] == '.') {
        return true;
    }
    
    return false;
}

// Helper for string buffer handling
typedef struct {
    char *buffer;
    size_t size;
    size_t capacity;
} StringBuffer;

// Initialize string buffer
static void init_string_buffer(StringBuffer *sb, size_t initial_capacity) {
    sb->buffer = (char *)malloc(initial_capacity);
    if (!sb->buffer) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    sb->buffer[0] = '\0';
    sb->size = 0;
    sb->capacity = initial_capacity;
}

// Append to string buffer
static void string_buffer_append(StringBuffer *sb, const char *str) {
    size_t len = strlen(str);
    size_t new_size = sb->size + len + 1;  // +1 for null terminator
    
    if (new_size > sb->capacity) {
        size_t new_capacity = sb->capacity * 2;
        while (new_capacity < new_size) {
            new_capacity *= 2;
        }
        
        sb->buffer = (char *)realloc(sb->buffer, new_capacity);
        if (!sb->buffer) {
            perror("Memory reallocation failed");
            exit(EXIT_FAILURE);
        }
        sb->capacity = new_capacity;
    }
    
    strcat(sb->buffer + sb->size, str);
    sb->size += len;
}

// Free string buffer
static char *string_buffer_release(StringBuffer *sb) {
    char *result = sb->buffer;
    sb->buffer = NULL;
    sb->size = 0;
    sb->capacity = 0;
    return result;
}

// Print tree to string buffer
static void print_tree_to_buffer(StringBuffer *sb, const char *dir, const char *prefix, 
                                const DirtreeConfig *config, int current_depth, 
                                VisitedDirs *visited) {
    // Check if we've reached the maximum depth
    if (config->max_depth > 0 && current_depth > config->max_depth) {
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
        
        // Check skip conditions
        bool is_directory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        if (is_directory && should_skip_dir(findData.cFileName, config)) {
            continue;
        }
        if (!is_directory && should_skip_file(findData.cFileName, config)) {
            continue;
        }
        
        // Create full path
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s\\%s", dir, findData.cFileName);
        
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
        
        items[count].path = (char *)strdup(path);
        items[count].name = (char *)strdup(findData.cFileName);
        items[count].is_dir = is_directory;
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
        
        // Create full path
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s/%s", dir, entry->d_name);
        
        // Check file type
        struct stat st;
        if (stat(path, &st) != 0) {
            continue;  // Skip if we can't stat the file
        }
        
        bool is_directory = S_ISDIR(st.st_mode);
        
        // Check skip conditions
        if (is_directory && should_skip_dir(entry->d_name, config)) {
            continue;
        }
        if (!is_directory && should_skip_file(entry->d_name, config)) {
            continue;
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
        
        items[count].path = (char *)strdup(path);
        items[count].name = (char *)strdup(entry->d_name);
        items[count].is_dir = is_directory;
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
            snprintf(current_prefix, PATH_MAX, "%s%s", prefix, TREE_CORNER(config->format));
            snprintf(next_prefix, PATH_MAX, "%s%s", prefix, TREE_SPACE(config->format));
        } else {
            snprintf(current_prefix, PATH_MAX, "%s%s", prefix, TREE_BRANCH(config->format));
            snprintf(next_prefix, PATH_MAX, "%s%s", prefix, TREE_VERTICAL(config->format));
        }
        
        // Print the current item
        char line[PATH_MAX + 100];
        snprintf(line, sizeof(line), "%s%s\n", current_prefix, items[i].name);
        string_buffer_append(sb, line);
        
        // Recursively process directories
        if (items[i].is_dir) {
            print_tree_to_buffer(sb, items[i].path, next_prefix, config, current_depth + 1, visited);
        }
        
        free(items[i].path);
        free(items[i].name);
    }
    
    free(items);
}

// Initialize the default configuration
void dirtree_init_config(DirtreeConfig *config) {
    if (!config) return;
    
    config->max_depth = -1;  // No limit by default
    config->skip_hidden = true;
    config->skip_common = true;
    
    // Use Unicode by default, fallback to ASCII on Windows
#ifdef _WIN32
    config->format = DIRTREE_FORMAT_ASCII;
#else
    config->format = DIRTREE_FORMAT_UNICODE;
#endif
    
    config->custom_skip_dirs = NULL;
    config->custom_skip_files = NULL;
}

// Free resources allocated for the configuration
void dirtree_free_config(DirtreeConfig *config) {
    if (!config) return;
    
    if (config->custom_skip_dirs) {
        for (int i = 0; config->custom_skip_dirs[i] != NULL; i++) {
            free(config->custom_skip_dirs[i]);
        }
        free(config->custom_skip_dirs);
        config->custom_skip_dirs = NULL;
    }
    
    if (config->custom_skip_files) {
        for (int i = 0; config->custom_skip_files[i] != NULL; i++) {
            free(config->custom_skip_files[i]);
        }
        free(config->custom_skip_files);
        config->custom_skip_files = NULL;
    }
}

// Add custom directory to skip
void dirtree_add_skip_dir(DirtreeConfig *config, const char *dirname) {
    if (!config || !dirname) return;
    
    int count = 0;
    if (config->custom_skip_dirs) {
        // Count existing entries
        while (config->custom_skip_dirs[count] != NULL) {
            count++;
        }
    }
    
    // Allocate or reallocate the array
    config->custom_skip_dirs = (char **)realloc(
        config->custom_skip_dirs, (count + 2) * sizeof(char *)
    );
    
    if (!config->custom_skip_dirs) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    
    // Add the new entry and NULL terminator
    config->custom_skip_dirs[count] = (char *)strdup(dirname);
    config->custom_skip_dirs[count + 1] = NULL;
}

// Add custom file to skip
void dirtree_add_skip_file(DirtreeConfig *config, const char *filename) {
    if (!config || !filename) return;
    
    int count = 0;
    if (config->custom_skip_files) {
        // Count existing entries
        while (config->custom_skip_files[count] != NULL) {
            count++;
        }
    }
    
    // Allocate or reallocate the array
    config->custom_skip_files = (char **)realloc(
        config->custom_skip_files, (count + 2) * sizeof(char *)
    );
    
    if (!config->custom_skip_files) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    
    // Add the new entry and NULL terminator
    config->custom_skip_files[count] = (char *)strdup(filename);
    config->custom_skip_files[count + 1] = NULL;
}

// Generate directory tree as string
char *dirtree_generate_string(const char *dirpath, DirtreeConfig *config) {
    if (!dirpath || !config) {
        return NULL;
    }
    
    // Convert to an absolute path
    char *abs_dir = get_absolute_path(dirpath);
    if (!abs_dir) {
        return NULL;
    }
    
    // Initialize string buffer
    StringBuffer sb;
    init_string_buffer(&sb, 4096);  // Start with 4KB buffer
    
    // Extract and print the root directory name
    char *base_name;
    
#ifdef _WIN32
    base_name = strrchr(abs_dir, '\\');
#else
    base_name = strrchr(abs_dir, '/');
#endif
    
    if (base_name == NULL) {
        base_name = abs_dir;
    } else {
        base_name++;  // Skip the separator
    }
    
    string_buffer_append(&sb, base_name);
    string_buffer_append(&sb, "\n");
    
    // Initialize visited directories
    VisitedDirs visited;
    init_visited_dirs(&visited, 100);
    
    // Generate the tree
    print_tree_to_buffer(&sb, abs_dir, "", config, 1, &visited);
    
    // Clean up
    free_visited_dirs(&visited);
    free(abs_dir);
    
    // Return the generated string
    return string_buffer_release(&sb);
}

// Print directory tree to specified file
int dirtree_print_to_file(FILE *output, const char *dirpath, DirtreeConfig *config) {
    if (!output || !dirpath || !config) {
        return -1;
    }
    
    char *tree_string = dirtree_generate_string(dirpath, config);
    if (!tree_string) {
        return -1;
    }
    
    int result = fputs(tree_string, output);
    free(tree_string);
    
    return (result >= 0) ? 0 : -1;
}

// Print directory tree to stdout
int dirtree_print(const char *dirpath, DirtreeConfig *config) {
    return dirtree_print_to_file(stdout, dirpath, config);
}

// Version information
const char *dirtree_version(void) {
    return DIRTREE_VERSION;
}

// Main function (only used for standalone binary, not when used as a library)
#ifndef DIRTREE_LIBRARY_ONLY
#include <getopt.h>

// Print help message
static void print_help(const char *program_name) {
    printf("Directory Tree Utility\n");
    printf("\n");
    printf("Usage: %s [options] [directory]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help               Display this help message and exit\n");
    printf("  -d, --depth=LEVEL        Maximum depth to display (default: no limit)\n");
    printf("  -a, --all                Disable skipping of common directories/files\n");
    printf("  -u, --unicode            Use Unicode characters for tree (default on Unix)\n");
    printf("  -A, --ascii              Use ASCII characters for tree (default on Windows)\n");
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
    printf("Library version: %s\n", dirtree_version());
    printf("\n");
}

int main(int argc, char *argv[]) {
    // Default values
    const char *dir = ".";
    DirtreeConfig config;
    dirtree_init_config(&config);
    
    // Define long options
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"depth", required_argument, 0, 'd'},
        {"all", no_argument, 0, 'a'},
        {"unicode", no_argument, 0, 'u'},
        {"ascii", no_argument, 0, 'A'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    // Parse options
    while ((c = getopt_long(argc, argv, "hd:auA", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print_help(argv[0]);
                dirtree_free_config(&config);
                return EXIT_SUCCESS;
            case 'd':
                config.max_depth = atoi(optarg);
                break;
            case 'a':
                config.skip_common = false;
                config.skip_hidden = false;
                break;
            case 'u':
                config.format = DIRTREE_FORMAT_UNICODE;
                break;
            case 'A':
                config.format = DIRTREE_FORMAT_ASCII;
                break;
            case '?':
                // getopt_long already printed an error message
                dirtree_free_config(&config);
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
        dirtree_free_config(&config);
        return EXIT_FAILURE;
    }
#else
    // Ensure DIR exists and is a directory
    struct stat st;
    if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a directory or doesn't exist.\n", dir);
        dirtree_free_config(&config);
        return EXIT_FAILURE;
    }
#endif

    // Print the tree
    int result = dirtree_print(dir, &config);
    
    // Clean up
    dirtree_free_config(&config);
    
    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif /* DIRTREE_LIBRARY_ONLY */
