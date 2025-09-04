#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Typewriter plugin - simulates typewriter effect by printing each character
 * with a 100ms delay
 */

/**
 * Plugin transformation function
 * Prints the string character by character with delay
 */
const char* plugin_transform(const char* input) {
    if (!input) {
        return NULL;
    }
    
    // Print plugin name with typewriter effect
    const char* plugin_name = "[typewriter] ";
    for (size_t i = 0; plugin_name[i] != '\0'; i++) {
        printf("%c", plugin_name[i]);
        fflush(stdout);  
        // 100ms delay
        usleep(100000);  
    }

    // Print input with typewriter effect
    for (size_t i = 0; input[i] != '\0'; i++) {
        printf("%c", input[i]);
        fflush(stdout);
        // 100ms delay
        usleep(100000); 
    }

    printf("\n");
    fflush(stdout);
    
    // Return a copy of the input
    return strdup(input);
}

/**
 * Initialize the plugin
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "typewriter", queue_size);
}