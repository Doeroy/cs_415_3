#ifndef THREADS_H
#define THREADS_H

#include "ordered_mutex.h"
#include <pthread.h>
#include <signal.h>

#define TIME_STR_BUF_SIZE 10

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond_passenger_boarded;
    pthread_cond_t cond_passenger_unboarded;
    int passengers_on_board;
    int car_id;
} Car;

typedef struct {
    int passenger_num;
    int car_capacity;
} passenger_info;

typedef struct {
    int car_id;
    int capacity;
    int ride_duration;
    int wait_period;
} car_info;

extern time_t start_time;
extern ordered_mutex_t ticket_queue_lock;
extern pthread_mutex_t loading_bay_lock;
extern ordered_mutex_t unloading_turnstile;
extern pthread_mutex_t passenger_coordination_lock;
extern pthread_cond_t cond_car_is_loading;
extern volatile sig_atomic_t shutdown_flag;
extern volatile int loading_car_id;
extern int g_num_cars;
extern Car* cars;

void* passenger_func(void* arg);
void* car_func(void* arg);
void get_time(char* buffer, size_t buf_size);

#endif