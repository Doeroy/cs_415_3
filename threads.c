#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "threads.h"
#include <time.h>
#include "queue.h"
#include <semaphore.h>


extern time_t start_time;

extern Queue * ticket_queue;

extern Queue* ride_queue;

extern pthread_mutex_t ticket_queue_mutex;

extern pthread_mutex_t ticket_booth_mutex;

extern pthread_mutex_t ride_queue_mutex;

extern sem_t load_sem;

void get_time(char *buffer, size_t buf_size){
    time_t curr_time = time(NULL);
    time_t output_time = curr_time - start_time;
    int hours = output_time / 3600;
    int minutes = (output_time % 3600) / 60;
    int seconds = output_time % 60;
    snprintf(buffer, buf_size, "%02d:%02d:%02d", hours, minutes, seconds);
}

void* passenger_func(void * arg){
    passenger_info * pass_info = (passenger_info *) arg;
    char time_buffer[TIME_STR_BUF_SIZE];
    get_time(time_buffer, sizeof(time_buffer));
    printf("[Time: %s] Passenger %d entered the park\n", time_buffer, pass_info->passenger_num);
    fflush(stdout);

    get_time(time_buffer, sizeof(time_buffer));
    printf("[Time: %s] Passenger %d is exploring the park...\n", time_buffer, pass_info->passenger_num);

    int max = 10;
    int min = 1;
    int wait_time = rand() % (max - min + 1) + min;
    sleep(wait_time);
    get_time(time_buffer, sizeof(time_buffer));
    printf("[Time: %s] Passenger %d finished exploring the park\n", time_buffer, pass_info->passenger_num);

    pthread_mutex_lock(&ticket_queue_mutex);
        enqueue(ticket_queue, pass_info->passenger_num);
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d waiting in ticket queue\n", time_buffer, pass_info->passenger_num);
    pthread_mutex_unlock(&ticket_queue_mutex);

    pthread_mutex_lock(&ticket_booth_mutex);
        pthread_mutex_lock(&ticket_queue_mutex);
            dequeue(ticket_queue);
        pthread_mutex_unlock(&ticket_queue_mutex);
            
        sleep(1);
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d acquired a ticket\n", time_buffer, pass_info->passenger_num);
    pthread_mutex_unlock(&ticket_booth_mutex);

    pthread_mutex_lock(&ride_queue_mutex);
        enqueue(ride_queue, pass_info->passenger_num);
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d joined the ride queue\n", time_buffer, pass_info->passenger_num);
    pthread_mutex_unlock(&ride_queue_mutex);

    board();

    free(pass_info);
    return NULL;
}

void * car_func(void * arg){
    car_info * info = (passenger_info *) arg;
    load(info->car_capacity);
}

void load(int capacity){
    for(int i = 0; i < capacity; i++)
        sem_post(&load_sem);
}

void board(){
    sem_wait(&load_sem);
}