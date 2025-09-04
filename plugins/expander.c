#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Expander plugin - inserts a single white space between each character
 */

/**
 * Plugin transformation function
 * Expands the string by adding spaces between characters
 */
const char* plugin_transform(const char* input) {
    if (!input) {
        return NULL;
    }
    
    size_t len = strlen(input);
    
    // Handle empty string - still allocate for consistency
    if (len == 0) {
        char* result = malloc(1);
        if (!result) {
            return NULL;
        }

        result[0] = '\0';
        
        return result;
    }

    // Calculate new length: original length + (length-1) spaces
    size_t new_len = len + (len > 0 ? len - 1 : 0);
    
    // Allocate memory for the result
    char* result = malloc(new_len + 1);
    if (!result) {
        return NULL;
    }
    
    // Build the expanded string
    size_t result_index = 0;
    for (size_t i = 0; i < len; i++) {
        result[result_index++] = input[i];
        
        // Add space after each character except the last one
        if (i < len - 1) {
            result[result_index++] = ' ';
        }
    }

    result[result_index] = '\0';
    
    return result;
}

/**
 * Initialize the plugin
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "expander", queue_size);
}