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

extern pthread_mutex_t ride_queue_mutex;

extern pthread_mutex_t board_counter_mutex;

extern pthread_mutex_t unboard_mutex;

extern pthread_mutex_t car_mutex;

extern sem_t load_sem; //number of open seats the car has

extern sem_t unload_sem;

extern sem_t * allAboard; 

extern sem_t * allUnboarded;

extern sem_t * loading_area;

extern sem_t * unloading_area;

extern sem_t ride_finished;

extern int passengers_boarded;

extern int passengers_unboarded;

int curr_car_id = 0;

int passengers_on_ride = 0;

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
        printf("[Time: %s] Passenger %d boarded Car %d\n", time_buffer, pass_info->passenger_num, (curr_car_id + 1));
        sem_wait(&unload_sem);
        pthread_mutex_lock(&unboard_mutex);
            int total_passengers = passengers_on_ride;
            unboard(total_passengers);
        pthread_mutex_unlock(&unboard_mutex);
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
        sem_wait(&loading_area[(info -> car_id)]);
        pthread_mutex_lock(&car_mutex);
            curr_car_id = info -> car_id;
        pthread_mutex_unlock(&car_mutex);

        load(info->car_capacity);
        get_time(time_buffer, sizeof(time_buffer));
        printf("[Time: %s] Car %d invoked load()\n", time_buffer, (info->car_id + 1));

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
               
        int result = sem_timedwait(&allAboard[info -> car_id], &abs_timeout_spec);
        pthread_mutex_lock(&board_counter_mutex);
            if(result == 0)
            {   
                passengers_on_ride = info->car_capacity;
                get_time(time_buffer, sizeof(time_buffer));
                printf("[Time: %s] Car %d is full with %d passengers.\n", time_buffer, (info->car_id + 1), passengers_on_ride);
                fflush(stdout);
            }
            else if (errno == ETIMEDOUT)
            {
                passengers_on_ride = passengers_boarded;
                passengers_boarded = 0;
            }
        pthread_mutex_unlock(&board_counter_mutex);
        sem_post(&loading_area[((info -> car_id) + 1) % (info -> total_amt_cars)]);
        //printf("passengers_on_ride: %d\n", passengers_on_ride);
        if(passengers_on_ride > 0){
            run(info->ride_duration);
            get_time(time_buffer, sizeof(time_buffer));
            printf("[Time: %s] Car %d finished its run\n", time_buffer, (info->car_id + 1));
            sem_wait(&unloading_area[(info -> car_id)]);
            unload(passengers_on_ride);
            for(int i = 0; i < passengers_on_ride; i++){
                sem_wait(&allUnboarded[info -> car_id]);
            }
            printf("here");
            //get_time(time_buffer, sizeof(time_buffer));
            //printf("[Time: %s] Passenger 1 unboarded Car %d\n", time_buffer, (info->car_id + 1));
            for(int i = 0; i < passengers_on_ride; i++) {
                sem_post(&ride_finished);
            }
            sem_post(&unloading_area[((info -> car_id) + 1) % (info -> total_amt_cars)]);
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
        pthread_mutex_lock(&car_mutex);
            int loading_id = curr_car_id;
        pthread_mutex_unlock(&car_mutex);
        sem_post(&allAboard[loading_id]); //car thread becomes unblocked and can now run 
        passengers_boarded = 0; //set passsengers_board to 0 so next passengers can board correctly
    }
    pthread_mutex_unlock(&board_counter_mutex); 
}

void unboard(int total_passengers)
{
    pthread_mutex_lock(&car_mutex);
        int unloading_id = curr_car_id;
    pthread_mutex_unlock(&car_mutex);
    for(int i = 0; i < total_passengers; i++)
    {
        sem_post(&allUnboarded[unloading_id]);
    }
}

void run(int ride_duration)
{
    sleep(ride_duration);
}