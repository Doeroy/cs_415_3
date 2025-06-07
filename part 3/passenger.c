#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "threads.h"

void* passenger_func(void *arg) {
    passenger_info* info = (passenger_info*)arg;
    char time_buffer[TIME_STR_BUF_SIZE];
    int passenger_id = info->passenger_num;
    int car_capacity = info->car_capacity;
    free(info);

    get_time(time_buffer, sizeof(time_buffer));
    printf("[Time: %s] Passenger %d entered the park.\n", time_buffer, passenger_id);

    while (!shutdown_flag) {
        pthread_mutex_lock(&shared_state->lock);
        shared_state->passengers_exploring++;
        pthread_mutex_unlock(&shared_state->lock);

        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d is exploring...\n", time_buffer, passenger_id); fflush(stdout);
        sleep((rand() % 4) + 2);

        if(shutdown_flag) break;

        pthread_mutex_lock(&shared_state->lock);
        shared_state->passengers_exploring--;
        pthread_mutex_unlock(&shared_state->lock);
        
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d waiting for ticket.\n", time_buffer, passenger_id); fflush(stdout);
        
        pthread_mutex_lock(&shared_state->lock);
        enqueue(&shared_state->ticket_queue, passenger_id);
        pthread_mutex_unlock(&shared_state->lock);
        
        ordered_mutex_lock(&ticket_queue_lock);
        if (shutdown_flag) { ordered_mutex_unlock(&ticket_queue_lock); break; }
        
        pthread_mutex_lock(&shared_state->lock);
        dequeue(&shared_state->ticket_queue);
        pthread_mutex_unlock(&shared_state->lock);
        
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d got a ticket.\n", time_buffer, passenger_id); fflush(stdout);
        ordered_mutex_unlock(&ticket_queue_lock);

        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d joined the ride queue.\n", time_buffer, passenger_id); fflush(stdout);
        
        pthread_mutex_lock(&shared_state->lock);
        enqueue(&shared_state->ride_queue, passenger_id);
        pthread_mutex_unlock(&shared_state->lock);

        int target_car_id = -1;
        int boarded_successfully = 0;

        while (!shutdown_flag && !boarded_successfully) {
            pthread_mutex_lock(&passenger_coordination_lock);
            while (!shutdown_flag && loading_car_id == -1) {
                pthread_cond_wait(&cond_car_is_loading, &passenger_coordination_lock);
            }
            if(shutdown_flag) { pthread_mutex_unlock(&passenger_coordination_lock); break; }
            target_car_id = loading_car_id;
            pthread_mutex_unlock(&passenger_coordination_lock);

            if (target_car_id == -1) continue;

            Car* car = &cars[target_car_id];
            pthread_mutex_lock(&car->lock);

            if (car->passengers_on_board < car_capacity) {
                boarded_successfully = 1;

                pthread_mutex_lock(&shared_state->lock);
                dequeue(&shared_state->ride_queue);
                shared_state->passengers_in_car[target_car_id]++;
                pthread_mutex_unlock(&shared_state->lock);

                car->passengers_on_board++;
                get_time(time_buffer, sizeof(time_buffer));
                printf("[Time: %s] Passenger %d boarded Car %d.\n", time_buffer, passenger_id, car->car_id); fflush(stdout);
                pthread_cond_signal(&car->cond_passenger_boarded);
                
                pthread_cond_wait(&car->cond_passenger_unboarded, &car->lock);
                
                if (!shutdown_flag) {
                    get_time(time_buffer, sizeof(time_buffer));
                    printf("[Time: %s] Passenger %d unboarded Car %d.\n", time_buffer, passenger_id, car->car_id);
                    fflush(stdout);
                }
            }
            pthread_mutex_unlock(&car->lock);

            if (!boarded_successfully) usleep(10000);
        }
    }
    return NULL;
}