#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Logger plugin - logs all strings that pass through to standard output
 */

/**
 * Plugin transformation function
 * Logs the input string and returns a copy
 */
const char* plugin_transform(const char* input) {
    if (!input) {
        return NULL;
    }

    // Log the input
    printf("[logger] %s\n", input);
    // Ensure immediate output
    fflush(stdout);
    
    // Return a copy of the input
    return strdup(input);
}

/**
 * Initialize the plugin
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "logger", queue_size);
}