/*
 * Copyright (c) 2025 Lucas Kafarski
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions of the BSD 3-Clause
 * License are met.
 */

#ifndef DIRTREE_H
#define DIRTREE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tree output format options
typedef enum {
    DIRTREE_FORMAT_ASCII = 0,    // ASCII characters for all platforms
    DIRTREE_FORMAT_UNICODE = 1   // Unicode characters when supported
} DirtreeFormat;

// Configuration options for directory tree traversal
typedef struct {
    int max_depth;               // Maximum depth (-1 for unlimited)
    bool skip_hidden;            // Skip hidden files and directories
    bool skip_common;            // Skip common directories like .git, node_modules, etc.
    DirtreeFormat format;        // Output format
    char **custom_skip_dirs;     // Additional directories to skip (NULL-terminated array)
    char **custom_skip_files;    // Additional files to skip (NULL-terminated array)
} DirtreeConfig;

// Initialize the default configuration
void dirtree_init_config(DirtreeConfig *config);

// Free resources allocated for the configuration
void dirtree_free_config(DirtreeConfig *config);

// Add custom directory to skip
void dirtree_add_skip_dir(DirtreeConfig *config, const char *dirname);

// Add custom file to skip
void dirtree_add_skip_file(DirtreeConfig *config, const char *filename);

// Generate directory tree as string
char *dirtree_generate_string(const char *dirpath, DirtreeConfig *config);

// Print directory tree to specified file (stdout, file, etc.)
int dirtree_print_to_file(FILE *output, const char *dirpath, DirtreeConfig *config);

// Print directory tree to stdout
int dirtree_print(const char *dirpath, DirtreeConfig *config);

// Version information
const char *dirtree_version(void);

#ifdef __cplusplus
}
#endif

#endif /* DIRTREE_H */
