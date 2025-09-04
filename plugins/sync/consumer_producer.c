#include "consumer_producer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Initialize a consumer-producer queue
 */
const char* consumer_producer_init(consumer_producer_t* queue, int capacity) {
    if (!queue) {
        return "Null queue pointer";
    }

    if (capacity <= 0) {
        return "Invalid capacity";
    }
    
    // Allocate items array
    queue->items = (char**)calloc(capacity, sizeof(char*));
    if (!queue->items) {
        return "Failed to allocate memory for queue items";
    }
    
    // Initialize queue parameters
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    
    // Initialize monitors
    if (monitor_init(&queue->not_full_monitor) != 0) {
        free(queue->items);
        queue->items = NULL;
        return "Failed to initialize not_full monitor";
    }
    
    if (monitor_init(&queue->not_empty_monitor) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);
        queue->items = NULL;
        return "Failed to initialize not_empty monitor";
    }
    
    if (monitor_init(&queue->finished_monitor) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        monitor_destroy(&queue->not_empty_monitor);
        free(queue->items);
        queue->items = NULL;
        return "Failed to initialize finished monitor";
    }

    if (pthread_mutex_init(&queue->lock, NULL) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        monitor_destroy(&queue->not_empty_monitor);
        monitor_destroy(&queue->finished_monitor);
        free(queue->items);
        queue->items = NULL; 
        return "Failed to initilize the lock";
    }

    return NULL;
}

/**
 * Destroy a consumer-producer queue and free its resources
 */
void consumer_producer_destroy(consumer_producer_t* queue) {
    if (!queue) {
        return;
    }
    
    // Free any remaining items
    if (queue->items) {
        for (int i = 0; i < queue->count; i++) {
            int index = (queue->head + i) % queue->capacity;
            free(queue->items[index]);
            queue->items[index] = NULL;
        }
        free(queue->items);
        queue->items = NULL;
    }
    
    // Destroy monitors
    monitor_destroy(&queue->not_full_monitor);
    monitor_destroy(&queue->not_empty_monitor);
    monitor_destroy(&queue->finished_monitor);
    pthread_mutex_destroy(&queue->lock);

    // Reset queue fields
    queue->capacity = 0;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;

    return;
}

/**
 * Add an item to the queue (producer)
 */
const char* consumer_producer_put(consumer_producer_t* queue, const char* item) {
    if (!queue) {
        return "Null queue pointer";
    }
    if (!item) {
        return "Null item pointer";
    }
    if (!queue->items) {
        return "Queue has been destroyed";
    }

    pthread_mutex_lock(&queue->lock);

    // Wait until queue is not full
    while (queue->count >= queue->capacity) {
        pthread_mutex_unlock(&queue->lock);
        if (monitor_wait(&queue->not_full_monitor) != 0) {
            return "Wait for not_full failed";
        }
        pthread_mutex_lock(&queue->lock);
    }

    // Duplicate the item to take ownership
    char* item_copy = strdup(item);
    if (!item_copy) {
        pthread_mutex_unlock(&queue->lock);
        return "Memory allocation failed for item";
    }
    
    // Add item to queue (make a copy to ensure ownership)
    queue->items[queue->tail] = item_copy;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    // Signal that queue is not empty
    monitor_signal(&queue->not_empty_monitor);

    if (queue->count < queue->capacity) {
        monitor_signal(&queue->not_full_monitor);
    }

    pthread_mutex_unlock(&queue->lock);
    
    return NULL;
}

/**
 * Remove an item from the queue (consumer)
 */
char* consumer_producer_get(consumer_producer_t* queue) {
    if (!queue) {
        return NULL;
    }

    pthread_mutex_lock(&queue->lock);

    // Wait until the queue is not empty or finished
    while (queue->count <= 0) {
        pthread_mutex_unlock(&queue->lock);
        if (monitor_wait(&queue->not_empty_monitor) != 0) {
            return NULL;
        }
        pthread_mutex_lock(&queue->lock);
    }

    // Get item from queue
    char* item = queue->items[queue->head];
    queue->items[queue->head] = NULL;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    // Signal that queue is not full
    monitor_signal(&queue->not_full_monitor);

    if (queue->count > 0) {
        monitor_signal(&queue->not_empty_monitor);
    }

    pthread_mutex_unlock(&queue->lock);
    
    return item;
}

/**
 * Signal that processing is finished
 */
void consumer_producer_signal_finished(consumer_producer_t* queue) {
    if (!queue) {
        return;
    }
    
    monitor_signal(&queue->finished_monitor);
}

/**
 * Wait for processing to be finished
 */
int consumer_producer_wait_finished(consumer_producer_t* queue) {
    if (!queue) {
        return -1;
    }
    
    return monitor_wait(&queue->finished_monitor);
}