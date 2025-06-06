#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>
#include <errno.h>
#include "threads.h"
#include "queue.h"


extern time_t start_time;

extern Queue * ticket_queue;

extern Queue* ride_queue;

extern pthread_mutex_t ticket_queue_mutex;

extern pthread_mutex_t ticket_booth_mutex;

extern pthread_mutex_t ride_queue_mutex;

extern pthread_mutex_t board_counter_mutex;

extern pthread_mutex_t unboard_counter_mutex;

extern sem_t load_sem; //number of open seats the car has

extern sem_t unload_sem;

extern sem_t allAboard; 

extern sem_t allUnboarded;

extern sem_t ride_finished;

extern int passengers_boarded;

extern int passengers_unboarded;

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
    while(1){
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

        board(pass_info -> car_capacity);
        printf("[Time: %s] Passenger %d boarded Car 1\n", time_buffer, pass_info->passenger_num);
        sem_wait(&unload_sem);
        unboard();
        sem_wait(&ride_finished);
    }   
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
        struct timespec abs_timeout_spec;
        if (clock_gettime(CLOCK_REALTIME, &abs_timeout_spec) == -1)
        {
            perror("clock_gettime");
        }
        else if((info -> total_passengers) == 1)
        {
            abs_timeout_spec.tv_sec += 1;
        }
        else
        {
            abs_timeout_spec.tv_sec += info->wait_period;
        }
        int passengers_on_ride = 0;
       
        int result = sem_timedwait(&allAboard, &abs_timeout_spec);
        pthread_mutex_lock(&board_counter_mutex);
            if(result == 0)
            {   
                passengers_on_ride = info->car_capacity;
                get_time(time_buffer, sizeof(time_buffer));
                printf("[Time: %s] Car %d is full with %d passengers.\n", time_buffer, info->car_id, passengers_on_ride);
                fflush(stdout);

            }
            else if (errno == ETIMEDOUT)
            {
                passengers_on_ride = passengers_boarded;
                passengers_boarded = 0;
                /*
                if (passengers_on_ride > 0) 
                {
                get_time(time_buffer, sizeof(time_buffer));
                printf("[Time: %s] Car %d departing due to timeout with %d passengers.\n", time_buffer, info->car_id, passengers_on_ride);
                fflush(stdout);
                }
                */
            }
        pthread_mutex_unlock(&board_counter_mutex);
        
        //printf("passengers_on_ride: %d\n", passengers_on_ride);
        if(passengers_on_ride > 0){
            run(info->ride_duration);
            unload(passengers_on_ride);
            for(int i = 0; i < passengers_on_ride; i++){
                sem_wait(&allUnboarded);
            }
            get_time(time_buffer, sizeof(time_buffer));
            printf("[Time: %s] Passenger 1 unboarded Car 1\n", time_buffer);
            sem_post(&ride_finished);
        }
    }
    free(info);
    return NULL;
}

void load(int capacity)
{
    for(int i = 0; i < capacity; i++)
    {
        sem_post(&load_sem); //increments the semaphore n times to let n passengers board
    }
}

void unload(int capacity)
{
    for(int i = 0; i < capacity; i ++)
    {
        sem_post(&unload_sem);
    }
}

void board(int car_capacity)
{
    sem_wait(&load_sem);
    pthread_mutex_lock(&board_counter_mutex); //mutex lock since this could cause a race condition
    passengers_boarded ++; //increment the number of passengers that have boarded
    if(passengers_boarded == car_capacity)
    {
        sem_post(&allAboard); //car thread becomes unblocked and can now run 
        passengers_boarded = 0; //set passsengers_board to 0 so next passengers can board correctly
    }
    pthread_mutex_unlock(&board_counter_mutex); 
}

void unboard()
{
    sem_post(&allUnboarded);
}

void run(int ride_duration)
{
    sleep(ride_duration);
}