#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

// Plugin interface function pointers
typedef const char* (*plugin_init_func_t)(int);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*plugin_place_work_func_t)(const char*);
typedef void (*plugin_attach_func_t)(const char* (*)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);

// Plugin handle structure
typedef struct {
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    plugin_place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    char* name;
    void* handle;
} plugin_handle_t;

// Global variables
static plugin_handle_t* plugins = NULL;
static int plugin_count = 0;

/**
 * Print usage information to stdout
 */
void print_usage(const char* program_name) {
    printf("Usage: %s <queue_size> <plugin1> <plugin2> ... <pluginN>\n", program_name);
    printf("Arguments:\n");
    printf("  queue_size    Maximum number of items in each plugin's queue\n");
    printf("  plugin1..N    Names of plugins to load (without .so extension)\n");
    printf("\n");
    printf("Available plugins:\n");
    printf("  logger        - Logs all strings that pass through\n");
    printf("  typewriter    - Simulates typewriter effect with delays\n");
    printf("  uppercaser    - Converts strings to uppercase\n");
    printf("  rotator       - Move every character to the right. Last character moves to the beginning.\n");
    printf("  flipper       - Reverses the order of characters\n");
    printf("  expander      - Expands each character with spaces\n");
    printf("\n");
    printf("Example:\n");
    printf("  %s 20 uppercaser rotator logger\n", program_name);
}

/**
 * Parse command line arguments
 * Returns queue_size on success, 1 on failure
 */
int parse_arguments(int argc, char* argv[], char*** plugin_names, int* num_plugins) {
    // Parse queue size
    char* endptr;
    int queue_size = (int)strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || queue_size <= 0) {
        fprintf(stderr, "Error: Invalid queue size\n");
        return -1;
    }
    
    // Get plugin names
    *num_plugins = argc - 2;
    if (num_plugins == 0) {
        fprintf(stderr, "Error: No plugins specified\n");
        return -1;
    }

    *plugin_names = &argv[2];
    
    return queue_size;
}

/**
 * Load a single plugin
 * Returns 0 on success, 1 on failure
 */
int load_plugin(const char* plugin_name, plugin_handle_t* plugin) {
    char filename[256];
    snprintf(filename, sizeof(filename), "./output/%s.so", plugin_name);
    
    // Load the shared object
    plugin->handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
    if (!plugin->handle) {
        fprintf(stderr, "Error loading plugin %s: %s\n", plugin_name, dlerror());
        return 1;
    }
    
    // Clear any existing error
    dlerror();
    
    // Load required functions
    plugin->init = (plugin_init_func_t)dlsym(plugin->handle, "plugin_init");
    plugin->fini = (plugin_fini_func_t)dlsym(plugin->handle, "plugin_fini");
    plugin->place_work = (plugin_place_work_func_t)dlsym(plugin->handle, "plugin_place_work");
    plugin->attach = (plugin_attach_func_t)dlsym(plugin->handle, "plugin_attach");
    plugin->wait_finished = (plugin_wait_finished_func_t)dlsym(plugin->handle, "plugin_wait_finished");    
    // Check for errors
    char* error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error resolving symbols in plugin %s: %s\n", plugin_name, error);
        dlclose(plugin->handle);
        plugin->handle = NULL;
        return 1;
    }
    
    // Verify all required functions are present
    if (!plugin->init || !plugin->fini || !plugin->place_work || 
        !plugin->attach || !plugin->wait_finished) {
        fprintf(stderr, "Error: Plugin %s missing required functions\n", plugin_name);
        dlclose(plugin->handle);
        plugin->handle = NULL;
        return 1;
    }
    
    // Store plugin name
    plugin->name = strdup(plugin_name);
    if (!plugin->name) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        dlclose(plugin->handle);
        plugin->handle = NULL;
        return 1;
    }
    
    return 0;
}

/**
 * Load all plugins
 * Returns 0 on success, 1 on failure
 */
int load_plugins(char** plugin_names, int num_plugins) {
    plugins = calloc(num_plugins, sizeof(plugin_handle_t));
    if (!plugins) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    plugin_count = num_plugins;
    
    for (int i = 0; i < num_plugins; i++) {
        if (load_plugin(plugin_names[i], &plugins[i]) != 0) {
            // Cleanup already loaded plugins
            for (int j = 0; j < i; j++) {
                if (plugins[j].handle) {
                    dlclose(plugins[j].handle);
                }
                free(plugins[j].name);
            }
            free(plugins);
            plugins = NULL;
            plugin_count = 0;
            return 1;
        }
    }
    
    return 0;
}

/**
 * Initialize all plugins
 * Returns 0 on success, -1 on failure
 */
int initialize_plugins(int queue_size) {
    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].init(queue_size);
        if (error){
            fprintf(stderr, "Error initializing plugin %s: %s\n", plugins[i].name, error);
            return -1;
        }
    }
    
    return 0;
}

/**
 * Attach plugins together in a chain
 */
void attach_plugins(void) {
    for (int i = 0; i < plugin_count - 1; i++) {
        plugins[i].attach(plugins[i + 1].place_work);
    }
    // Last plugin is not attached to anything
}

/**
 * Process input from stdin
 * Returns 0 on success, -1 on failure
 */
int process_input(void) {
    char line[1025]; // 1024 characters + null terminator
    
    while (fgets(line, sizeof(line), stdin) != NULL) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Send to first plugin with error checking
        const char* error = plugins[0].place_work(line);
        if (error != NULL) {
            fprintf(stderr, "Error processing input '%s': %s\n", line, error);
            return -1;
        }
        
        // Check for termination signal
        if (strcmp(line, "<END>") == 0) {
            break;
        }
    }
    
    return 0;
}

/**
 * Wait for all plugins to finish processing
 * Returns 0 on success, -1 on failure
 */
int wait_for_plugins(void) {
    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].wait_finished();
        if (error != NULL) {
            fprintf(stderr, "Error waiting for plugin %s: %s\n", plugins[i].name, error);
            return -1;
        }
    }
    
    return 0;
}

/**
 * Clean up all plugins
 */
void cleanup_plugins(void) {
    if (plugins) {
        for (int i = 0; i < plugin_count; i++) {
            if (plugins[i].fini) {
                const char* error = plugins[i].fini();
                if (error != NULL) {
                    fprintf(stderr, "Warning: Error in plugin cleanup for %s: %s\n", 
                           plugins[i].name ? plugins[i].name : "unknown", error);
                }
            }
            if (plugins[i].handle) {
                dlclose(plugins[i].handle);
                plugins[i].handle = NULL;
            }
            if (plugins[i].name) {
                free(plugins[i].name);
                plugins[i].name = NULL;
            }
        }
        free(plugins);
        plugins = NULL;
    }
    plugin_count = 0;
}

/**
 * Check for duplicate plugin names
 * Returns 0 if no duplicates found, 1 if duplicates exist
 */
int check_duplicate_plugins(char** plugin_names, int num_plugins) {
    for (int i = 0; i < num_plugins; i++) {
        for (int j = i + 1; j < num_plugins; j++) {
            if (strcmp(plugin_names[i], plugin_names[j]) == 0) {
                fprintf(stderr, "Error: Duplicate plugin '%s' found\n", plugin_names[i]);
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Main function
 */
int main(int argc, char* argv[]) {
    char** plugin_names;
    int num_plugins;
    int queue_size;
    
    // Step 1: Parse command line arguments
    if (argc < 3) {
        fprintf(stderr, "Error: Insufficient arguments\n");
        print_usage(argv[0]); 
        return 1;
    }

    queue_size = parse_arguments(argc, argv, &plugin_names, &num_plugins);
    if (queue_size < 0 || num_plugins < 0) {
        print_usage(argv[0]);
        return 1;
    }

    if (check_duplicate_plugins(plugin_names, num_plugins) != 0) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Step 2: Load plugin shared objects
    if (load_plugins(plugin_names, num_plugins) != 0) {
        cleanup_plugins();
        print_usage(argv[0]);
        return 1;
    }
    
    // Step 3: Initialize plugins
    if (initialize_plugins(queue_size) != 0) {
        cleanup_plugins();
        return 2;
    }
    
    // Step 4: Attach plugins together
    attach_plugins();
    
    // Step 5: Read input from STDIN
    if (process_input() != 0) {
        cleanup_plugins();
    }
    
    // Step 6: Wait for plugins to finish
    if (wait_for_plugins() != 0) {
        cleanup_plugins();
    }
    
    // Step 7: Cleanup
    cleanup_plugins();
    
    // Step 8: Finalize
    printf("Pipeline shutdown complete\n");
    
    return 0;
}