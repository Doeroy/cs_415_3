#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <errno.h>
#include "ordered_mutex.h"
#include "threads.h"
#include "queue.h"

extern time_t start_time;

extern Queue* ride_queue;

extern int boarders;

extern int unboarders;

extern sem_t boardQueue;

extern sem_t unboardQueue;

extern sem_t allAboard; 

extern sem_t allAshore;

extern sem_t boardingSignal;

void get_time(char *buffer, size_t buf_size){
    time_t curr_time = time(NULL);
    time_t output_time = curr_time - start_time;
    int hours = output_time / 3600;
    int minutes = (output_time % 3600) / 60;
    int seconds = output_time % 60;
    snprintf(buffer, buf_size, "%02d:%02d:%02d", hours, minutes, seconds);
}

void* passenger_func(void * arg){
    passenger_info * pass_info = (passenger_info *) arg; //gets the passenger_info struct
    char time_buffer[TIME_STR_BUF_SIZE];
    get_time(time_buffer, sizeof(time_buffer)); //gets the current time
    printf("[Time: %s] Passenger %d entered the park\n", time_buffer, pass_info->passenger_num);
    fflush(stdout);
    while(1){
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d is exploring the park...\n", time_buffer, pass_info->passenger_num);
        ordered_mutex_t ticket_booth;
        int max = 10;
        int min = 1;
        int wait_time = rand() % (max - min + 1) + min; //gets a random wait time for the sleep
        sleep(wait_time);
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d finished exploring the park\n", time_buffer, pass_info->passenger_num);
        ordered_mutex_lock(&ticket_booth);
            get_time(time_buffer, sizeof(time_buffer));
            printf("[Time: %s] Passenger %d waiting in ticket queue\n", time_buffer, pass_info->passenger_num);
            sleep(1);
            get_time(time_buffer, sizeof(time_buffer));
            printf("[Time: %s] Passenger %d acquired a ticket\n", time_buffer, pass_info->passenger_num);
        ordered_mutex_unlock(&ticket_booth);
        enqueue(ride_queue, pass_info->passenger_num);
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Passenger %d joined the ride queue\n", time_buffer, pass_info->passenger_num);
    }
    sem_wait(&boardingSignal);
    sem_wait(&boardQueue);
    board();
    free(pass_info);
    return NULL;
}


void * car_func(void * arg)
{
    car_info * info = (car_info *) arg;
    char time_buffer[TIME_STR_BUF_SIZE];
    while(1)
    {
        load(info->car_capacity);   
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Car %d invoked load()\n", time_buffer, (info->car_id));
        sem_post(&boardingSignal);
    }
    free(info);
    return NULL;
}
/*
void load(int capacity)
{

}

void unload(int capacity)
{
    
}

void board(int car_capacity)
{
    
}

void unboard(int total_passengers)
{
   
}

void run(int ride_duration)
{
    
}
*/