#include "ordered_mutex.h"
#include "threads.h"      // <-- ADDED THIS INCLUDE
#include <stdio.h>

void ordered_mutex_init(ordered_mutex_t *om) {
    pthread_mutex_init(&om->mutex, NULL);
    pthread_cond_init(&om->cond, NULL);
    om->now_serving = 0;
    om->next_ticket = 0;
}

void ordered_mutex_lock(ordered_mutex_t *om) {
    pthread_mutex_lock(&om->mutex);
    // Check for shutdown signal right after acquiring the internal lock
    if (shutdown_flag) {
        pthread_mutex_unlock(&om->mutex);
        return; // Exit immediately if shutdown is in progress
    }
    unsigned long my_ticket = om->next_ticket++;
    while (!shutdown_flag && my_ticket != om->now_serving) {
        pthread_cond_wait(&om->cond, &om->mutex);
    }
    pthread_mutex_unlock(&om->mutex);
}

void ordered_mutex_unlock(ordered_mutex_t *om) {
    pthread_mutex_lock(&om->mutex);
    om->now_serving++;
    pthread_cond_broadcast(&om->cond);
    pthread_mutex_unlock(&om->mutex);
}

void ordered_mutex_destroy(ordered_mutex_t *om) {
    pthread_mutex_destroy(&om->mutex);
    pthread_cond_destroy(&om->cond);
}

// Wakes up all threads sleeping inside the ordered_mutex_lock function.
void ordered_mutex_broadcast(ordered_mutex_t *om) {
    pthread_mutex_lock(&om->mutex);
    pthread_cond_broadcast(&om->cond);
    pthread_mutex_unlock(&om->mutex);
}