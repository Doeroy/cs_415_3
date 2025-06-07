#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ordered_mutex.h"
// Function to initialize our ordered mutex
void ordered_mutex_init(ordered_mutex_t *om) {
    pthread_mutex_init(&om->mutex, NULL);
    pthread_cond_init(&om->cond, NULL);
    om->now_serving = 0;
    om->next_ticket = 0;
}

// Function to lock the ordered mutex
void ordered_mutex_lock(ordered_mutex_t *om) {
    // Lock the internal mutex to get a ticket
    pthread_mutex_lock(&om->mutex);
    
    // Get my ticket number
    unsigned long my_ticket = om->next_ticket++;
    
    // Wait while it's not my turn.
    // pthread_cond_wait will atomically unlock the mutex and wait.
    // When it wakes up, it re-acquires the mutex before returning.
    while (my_ticket != om->now_serving) {
        pthread_cond_wait(&om->cond, &om->mutex);
    }
    
    // It's my turn now. I still hold the internal mutex, so I will
    // unlock it before returning, allowing the next thread to get a ticket.
    // The "lock" is now conceptually held by this thread.
    pthread_mutex_unlock(&om->mutex);
}

// Function to unlock the ordered mutex
void ordered_mutex_unlock(ordered_mutex_t *om) {
    // Lock the internal mutex to update the serving counter
    pthread_mutex_lock(&om->mutex);
    
    // Increment the serving counter to signal the next turn
    om->now_serving++;
    
    // Wake up all waiting threads. Only one will have the correct ticket.
    pthread_cond_broadcast(&om->cond);
    
    // Unlock the internal mutex
    pthread_mutex_unlock(&om->mutex);
}

// Function to destroy the ordered mutex
void ordered_mutex_destroy(ordered_mutex_t *om) {
    pthread_mutex_destroy(&om->mutex);
    pthread_cond_destroy(&om->cond);
}