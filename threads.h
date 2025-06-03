#ifndef THREADS_H 
#define THREADS_H 
#define TIME_STR_BUF_SIZE 10
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct passenger_info{
    int passenger_num;
} passenger_info;

typedef struct car_info{
    int car_capacity;
    int ride_duration;
} car_info;

void get_time(char *buffer, size_t buf_size); 

void* passenger_func(void * arg);

void* car_func(void * arg);

void load(int capacity);

void board();

#endif 