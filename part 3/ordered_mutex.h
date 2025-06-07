#ifndef ORDERED_MUTEX_H
#define ORDERED_MUTEX_H

#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned long now_serving;
    unsigned long next_ticket;
} ordered_mutex_t;

void ordered_mutex_init(ordered_mutex_t *om);
void ordered_mutex_lock(ordered_mutex_t *om);
void ordered_mutex_unlock(ordered_mutex_t *om);
void ordered_mutex_destroy(ordered_mutex_t *om);
void ordered_mutex_broadcast(ordered_mutex_t *om);

#endif