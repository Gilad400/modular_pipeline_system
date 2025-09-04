#include "monitor.h"
#include <stdio.h>

/**
 * Initialize a monitor
 */
int monitor_init(monitor_t* monitor) {
    if (!monitor) {
        return -1;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&monitor->mutex, NULL) != 0) {
        return -1;
    }
    
    // Initialize condition variable
    if (pthread_cond_init(&monitor->condition, NULL) != 0) {
        pthread_mutex_destroy(&monitor->mutex);
        return -1;
    }
    
    // Initialize signaled flag
    monitor->signaled = 0;
    
    return 0;
}

/**
 * Destroy a monitor and free its resources
 */
void monitor_destroy(monitor_t* monitor) {
    if (!monitor) {
        return;
    }
    
    pthread_mutex_destroy(&monitor->mutex);
    pthread_cond_destroy(&monitor->condition);
}

/**
 * Signal a monitor (sets the monitor state)
 */
void monitor_signal(monitor_t* monitor) {
    if (!monitor) {
        return;
    }
    
    pthread_mutex_lock(&monitor->mutex);
    
    // Set the signaled flag
    monitor->signaled = 1;
    
    // Wake up waiting thread
    pthread_cond_signal(&monitor->condition);

    pthread_mutex_unlock(&monitor->mutex);
}

/**
 * Reset a monitor (clears the monitor state)
 */
void monitor_reset(monitor_t* monitor) {
    if (!monitor) {
        return;
    }
    
    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 0;
    pthread_mutex_unlock(&monitor->mutex);
}

/**
 * Wait for a monitor to be signaled (infinite wait)
 */
int monitor_wait(monitor_t* monitor) {
    if (!monitor) {
        return -1;
    }
    
    if (pthread_mutex_lock(&monitor->mutex) != 0) {
        return -1;
    }
    
    // Wait while not signaled
    while (monitor->signaled == 0) {
        pthread_cond_wait(&monitor->condition, &monitor->mutex);
    }

    monitor->signaled = 0;
    pthread_mutex_unlock(&monitor->mutex);
    
    return 0;
}