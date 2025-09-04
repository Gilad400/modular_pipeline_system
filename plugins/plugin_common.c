#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static plugin_context_t* plugin_context = NULL;

/**
 * Print error message in the format [ERROR][Plugin Name] - message
 */
void log_error(plugin_context_t* context, const char* message) {
    // if (!context || !message) {
    //     fprintf(stderr, "[ERROR][Unknown] - Invalid arguments\n");
    //     return;
    // }
    // fprintf(stderr, "[ERROR][%s] - %s\n", context->name, message);
}

/**
 * Print info message in the format [INFO][Plugin Name] - message
 */
void log_info(plugin_context_t* context, const char* message) {
    // if (!context || !message) {
    //     fprintf(stderr, "[INFO][Unknown] - Invalid arguments\n");
    //     return;
    // }
    // fprintf(stderr, "[INFO][%s] - %s\n", context->name, message);
}

/**
 * Generic consumer thread function
 */
void* plugin_consumer_thread(void* arg) {
    plugin_context_t* context = (plugin_context_t*)arg;
    if (!context || !context->queue || !context->process_function) {
        log_error(NULL, "Invalid context in consumer thread");
        return NULL;
    }

    log_info(context, "Consumer thread started");

    while (!context->finished) {
        // Get item from queue
        char* item = consumer_producer_get(context->queue);
        if (!item) {
            // Queue is empty but not finished, keep waiting
            continue; 
        }

        if (strcmp(item, "<END>") == 0) {
            log_info(context, "Received end signal, finishing the plugin");
            if (context->next_place_work) {
                context->next_place_work("<END>");
            }

            context->finished = 1;
            consumer_producer_signal_finished(context->queue);
            free(item);

            break;
        }

        const char* result = context->process_function(item);
        if (!result) {
            log_error(context, "Processing function returned NULL");
            free(item);

            continue;
        }

        if (context->next_place_work) {
            const char* next_result = context->next_place_work(result);
            if (next_result) {
                log_error(context, "Failed to call next_place_work");
            }
        }

        if (result != item) {
            free((void*)result);
        }
        
        free(item);
        log_info(context, "Processed item successfully");
    }

    log_info(context, "Consumer thread exiting");
    return NULL;
}


/**
 * Get the plugin's name
 */
const char* plugin_get_name(void) {
    if (!plugin_context || !plugin_context->name) {
        return "Plugin context is not initialized";
    }
    return plugin_context->name;
}

/**
 * Initialize the common plugin infrastructure
 */
const char* common_plugin_init(const char* (*process_function)(const char*), const char* name, int queue_size) {
    if (!process_function || !name || queue_size <= 0) {
        return "Invalid arguments";
    }

    if (plugin_context && plugin_context->initialized) {
        return "Plugin is already initialized";
    }

    plugin_context = malloc(sizeof(plugin_context_t));
    if (!plugin_context) {
        return "Failed to allocate memory for plugin context";
    }

    plugin_context->name = strdup(name);
    if (!plugin_context->name) {
        free(plugin_context);
        plugin_context = NULL;

        return "Failed to copy plugin name";
    }

    plugin_context->queue = malloc(sizeof(consumer_producer_t));
    if (!plugin_context->queue) {
        free((void*)plugin_context->name);
        free(plugin_context);
        plugin_context = NULL;

        return "Failed to allocate memory for queue";
    }

    // Initialize queue
    const char* result = consumer_producer_init(plugin_context->queue, queue_size);
    if (result) {
        free(plugin_context->queue);
        free((void*)plugin_context->name);
        free(plugin_context);
        plugin_context = NULL;

        return result;
    }

    plugin_context->next_place_work = NULL;
    plugin_context->process_function = process_function;
    plugin_context->initialized = 1;
    plugin_context->finished = 0;

    // Start consumer thread
    if (pthread_create(&plugin_context->consumer_thread, NULL, plugin_consumer_thread, plugin_context) != 0) {
        consumer_producer_destroy(plugin_context->queue);
        free(plugin_context->queue);
        free((void*)plugin_context->name);
        free(plugin_context);
        plugin_context = NULL;

        return "Failed to create consumer thread";
    }

    log_info(plugin_context, "Plugin initialized successfully");
    return NULL;
}

/**
 * Finalize the plugin
 */
const char* plugin_fini(void) {
    if (!plugin_context || !plugin_context->initialized || !plugin_context->queue) {
        return "Plugin is not initialized";
    }

    // Signal queue to finish
    consumer_producer_signal_finished(plugin_context->queue);

    // Wait for consumer thread to finish
    if (pthread_join(plugin_context->consumer_thread, NULL) != 0) {
        log_error(plugin_context, "Failed to join consumer thread");
        return "Failed to join consumer thread";
    }

    // Clean up queue
    if (plugin_context->queue)
    {
        consumer_producer_destroy(plugin_context->queue);
        free(plugin_context->queue);
    }

    // Free name
    if (plugin_context->name) {
        free((char*)plugin_context->name);
    }

    plugin_context->initialized = 0;
    free(plugin_context);
    plugin_context = NULL;

    return NULL;
}

/**
 * Place work into the plugin's queue
 */
const char* plugin_place_work(const char* str) {
    if (!plugin_context || !plugin_context->queue || !plugin_context->initialized) {
        return "Plugin is not initialized";
    }
    if (!str) {
        return "Input string cannot be NULL";
    }

    const char* result = consumer_producer_put(plugin_context->queue, str);
    if (result) {
        return result;
    }

    log_info(plugin_context, "Placed work in the queue successfully");
    
    return NULL;
}

/**
 * Attach this plugin to the next plugin
 */
void plugin_attach(const char* (*next_place_work)(const char*)) {
    if (!plugin_context) {
        log_error(NULL, "Plugin context is not initialized");
        return;
    }

    if (!next_place_work) {
        log_error(plugin_context, "Next place work function cannot be NULL");
        return;
    }

    plugin_context->next_place_work = next_place_work;
    log_info(plugin_context, "Successfully attached to the next plugin");
}

/**
 * Wait until the plugin has finished processing
 */
const char* plugin_wait_finished(void) {
    if (!plugin_context || !plugin_context->queue || !plugin_context->initialized) {
        return "Plugin is not initialized";
    }

    if (consumer_producer_wait_finished(plugin_context->queue) != 0) {
        log_error(plugin_context, "Failed to wait for processing to finish");
        return "Failed to wait for processing to finish";
    }

    log_info(plugin_context, "Processing finished successfully");
    
    return NULL; 
}