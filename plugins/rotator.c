#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Rotator plugin - moves every character one position to the right
 * The last character wraps around to the front
 */

/**
 * Plugin transformation function
 * Rotates the string one character to the right
 */
const char* plugin_transform(const char* input) {
    if (!input) {
        return NULL;
    }
    
    size_t len = strlen(input);
    
    // Handle empty string or single character
    if (len <= 1) {
        return strdup(input);
    }
    
    // Allocate memory for the result
    char* result = malloc(len + 1);
    if (!result) {
        return NULL;
    }
    
    // Rotate: move last character to front, shift others right
    result[0] = input[len - 1];
    for (size_t i = 1; i < len; i++) {
        result[i] = input[i - 1];
    }
    
    result[len] = '\0';
    
    return result;
}

/**
 * Initialize the plugin
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "rotator", queue_size);
}