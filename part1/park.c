#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "threads.h"
#include "queue.h"
#include <time.h>
#include <semaphore.h>

time_t start_time; 

Queue* ride_queue;

int boarders = 0;

int unboarders = 0;

sem_t boardQueue;

sem_t unboardQueue;

sem_t allAboard; //binary semaphore to signal that the car is full

sem_t allAshore; //binary semaphore to signal that all passengers have left the car

sem_t boarding;
int main(int argc, char * argv[]){
    int opt;
    int num_passengers = 1;
    int num_cars = 1;
    int cap_cars = 5;
    int wait_period = 3;
    int ride_duration = 1;
	sem_init(&boardQueue, 0, 0);
	sem_init(&unboardQueue, 0, 0);
	sem_init(&allAboard, 0, 0);
    sem_init(&allAshore, 0, 0);
	ride_queue = createQueue();
	start_time = time(NULL);
	srand(time(NULL));
    while((opt = getopt(argc, argv,":n:c:p:w:r:h")) != -1){
	switch(opt)
	{
	case 'n':
		num_passengers = atoi(optarg);
		break;
	case 'c':
		num_cars = atoi(optarg);
		break;
	case 'p':
		cap_cars = atoi(optarg);
		break;
	case 'w':
		wait_period = atoi(optarg);
		break;
	case 'r':
		ride_duration = atoi(optarg);
		break;
	case 'h':
		printf("Help Menu\n");
		break;
	case ':':
		printf("Option needs a value\n");
		break;
	case '?':
		printf("unknown option: %c\n", optopt);
		break;
	}
    }   
    for(; optind < argc; optind++){ //when some extra arguments are passed
	printf("Given extra arguments: %s\n", argv[optind]);
    }

    printf("===== DUCK PARK SIMULATION =====\n");
    printf("[Monitor] Simulation started with parameters:\n");
    printf("- Number of passenger threads: %d\n", num_passengers);
    printf("- Number of cars: %d\n", num_cars);
    printf("- Capacity per car: %d\n",cap_cars);
    printf("- Car waiting period: %d seconds\n", wait_period); 
    printf("- Ride duration: %d seconds\n", ride_duration); //need \n or fflush(stdout) to force buffer to flush (appear on screen)

	pthread_t * passenger_id_arr = (pthread_t *)malloc(num_passengers * sizeof(pthread_t));
	for(int i = 0; i < num_passengers; i++){
		passenger_info * info = (passenger_info *)malloc(sizeof(passenger_info)); 
		info->passenger_num = i + 1;
		info->car_capacity = cap_cars;
		pthread_create(&passenger_id_arr[i], NULL, passenger_func, (void*)(info));
	}

	pthread_t * car_id_arr = (pthread_t *)malloc(num_cars * sizeof(pthread_t));
	for(int i = 0; i < num_cars; i++){
		car_info * info = (car_info *)malloc(sizeof(car_info)); 
		info->car_capacity = cap_cars;
		info->ride_duration = ride_duration;
		info->wait_period = wait_period;
		info -> car_id = i + 1;
		info -> total_passengers = num_passengers;
		pthread_create(&car_id_arr[i], NULL, car_func, (void*)(info));
	}

	for(int j = 0; j < num_passengers; j++){
		pthread_join(passenger_id_arr[j], NULL);
	}

	for(int j = 0; j < num_cars; j++){
		pthread_join(car_id_arr[j], NULL);
	}

	sem_destroy(&boardQueue);
	sem_destroy(&unboardQueue);
	sem_destroy(&allAboard);
	sem_destroy(&allAboard);
	sem_destroy(&allAshore);
	//free(allAboard_sems);
	//free(allUnboarded_sems);
	free(passenger_id_arr);
	free(car_id_arr);
	free(ride_queue);
}   