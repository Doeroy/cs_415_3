#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct {
    pthread_mutex_t mutex;      // Internal mutex for protecting the ticket counters
    pthread_cond_t cond;        // Condition variable for threads to wait on
    unsigned long now_serving;  // The ticket number that is currently being served
    unsigned long next_ticket;  // The ticket number for the next waiting thread
} ordered_mutex_t;

// Function to initialize our ordered mutex
void ordered_mutex_init(ordered_mutex_t *om);

void ordered_mutex_lock(ordered_mutex_t *om);

void ordered_mutex_unlock(ordered_mutex_t *om);

void ordered_mutex_destroy(ordered_mutex_t *om);

