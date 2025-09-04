#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Uppercaser plugin - converts all alphabetic characters to uppercase
 */

/**
 * Plugin transformation function
 * Converts input string to uppercase
 */
const char* plugin_transform(const char* input) {
    if (!input) {
        return NULL;
    }
    
    // Allocate memory for the result
    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (!result) {
        return NULL;
    }
    
    // Convert to uppercase
    for (size_t i = 0; i < len; i++) {
        result[i] = (char)toupper((unsigned char)input[i]);
    }
    
    result[len] = '\0';
    
    return result;
}

/**
 * Initialize the plugin
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "uppercaser", queue_size);
}