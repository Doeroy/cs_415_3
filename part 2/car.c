#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "threads.h"

void* car_func(void* arg) {
    car_info* info = (car_info*)arg;
    int car_id = info->car_id;
    int capacity = info->capacity;
    int ride_duration = info->ride_duration;
    int wait_period = info->wait_period;
    Car* self = &cars[car_id];
    char time_buffer[TIME_STR_BUF_SIZE];
    free(info);

    while (!shutdown_flag) {
        pthread_mutex_lock(&loading_bay_lock);
        if (shutdown_flag) { pthread_mutex_unlock(&loading_bay_lock); break; }

        pthread_mutex_lock(&passenger_coordination_lock);
        loading_car_id = car_id;
        pthread_mutex_unlock(&passenger_coordination_lock);

        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Car %d is in the loading bay (waiting %ds).\n", time_buffer, car_id, wait_period);
        fflush(stdout);
        pthread_cond_broadcast(&cond_car_is_loading);

        pthread_mutex_lock(&self->lock);
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += wait_period;
        
        while (!shutdown_flag && self->passengers_on_board < capacity) {
            int result = pthread_cond_timedwait(&self->cond_passenger_boarded, &self->lock, &timeout);
            if (result == ETIMEDOUT) {
                get_time(time_buffer, sizeof(time_buffer));
                printf("[Time: %s] Car %d wait timer expired.\n", time_buffer, car_id); fflush(stdout);
                break;
            }
        }
        
        int is_running = (self->passengers_on_board > 0);
        pthread_mutex_unlock(&self->lock);

        pthread_mutex_lock(&passenger_coordination_lock);
        loading_car_id = -1;
        pthread_mutex_unlock(&passenger_coordination_lock);
        
        pthread_mutex_unlock(&loading_bay_lock);

        if (is_running) {
            get_time(time_buffer, sizeof(time_buffer));
            printf("[Time: %s] Car %d departed with %d passengers.\n", time_buffer, car_id, self->passengers_on_board); fflush(stdout);
            
            for (int i = 0; i < ride_duration; i++) {
                if (shutdown_flag) break;
                sleep(1);
            }
            if (shutdown_flag) break;

            ordered_mutex_lock(&unloading_turnstile);
            if (shutdown_flag) { ordered_mutex_unlock(&unloading_turnstile); break; }
            
            get_time(time_buffer, sizeof(time_buffer));
            printf("[Time: %s] Car %d finished run, now unloading.\n", time_buffer, car_id); fflush(stdout);
            
            pthread_mutex_lock(&self->lock);
            pthread_cond_broadcast(&self->cond_passenger_unboarded);
            self->passengers_on_board = 0;
            pthread_mutex_unlock(&self->lock);

            ordered_mutex_unlock(&unloading_turnstile);
        } else {
            get_time(time_buffer, sizeof(time_buffer));
            printf("[Time: %s] Car %d had no passengers.\n", time_buffer, car_id); fflush(stdout);
        }
        printf("\n");
    }
    return NULL;
}