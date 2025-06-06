#ifndef THREADS_H 
#define THREADS_H 
#define TIME_STR_BUF_SIZE 10
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct passenger_info{
    int passenger_num;
    int car_capacity;
} passenger_info;

typedef struct car_info{
    int car_capacity;
    int ride_duration;
    int wait_period;
    int car_id; 
    int total_passengers;
} car_info;

void get_time(char *buffer, size_t buf_size); 

void* passenger_func(void * arg);

void* car_func(void * arg);

void load(int capacity);

void unload(int capacity);

void board(int capacity);

void unboard();

void run(int ride_duration);

#endif 