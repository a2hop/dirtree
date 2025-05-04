/*
 * Example use of the dirtree library
 */

#include <stdio.h>
#include <stdlib.h>
#include "dirtree.h"

int main(int argc, char *argv[]) {
    // Path to analyze (use command line argument or default)
    const char *path = (argc > 1) ? argv[1] : ".";
    
    // Initialize configuration with defaults
    DirtreeConfig config;
    dirtree_init_config(&config);
    
    // Customize configuration
    config.max_depth = 3;                    // Limit depth to 3 levels
    config.format = DIRTREE_FORMAT_UNICODE;  // Use Unicode characters
    
    // Add custom directories/files to skip
    dirtree_add_skip_dir(&config, "build");
    dirtree_add_skip_file(&config, "README.md");
    
    // Print version
    printf("Using dirtree library version: %s\n\n", dirtree_version());
    
    // Print the directory tree to stdout
    printf("Directory tree for: %s\n", path);
    dirtree_print(path, &config);
    
    // Clean up
    dirtree_free_config(&config);
    
    return 0;
}
