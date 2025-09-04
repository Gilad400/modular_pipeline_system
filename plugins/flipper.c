#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Flipper plugin - reverses the order of characters in the string
 */

/**
 * Plugin transformation function
 * Reverses the input string
 */
const char* plugin_transform(const char* input) {
    if (!input) {
        return NULL;
    }
    
    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (!result) {
        return NULL;
    }
    
    // Reverse the string
    for (size_t i = 0; i < len; i++) {
        result[i] = input[len - 1 - i];
    }
    
    result[len] = '\0';
    
    return result;
}

/**
 * Initialize the plugin
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "flipper", queue_size);
}