#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "threads.h"

// Global Variable Definitions
time_t start_time;
ordered_mutex_t ticket_queue_lock;
pthread_mutex_t loading_bay_lock;
ordered_mutex_t unloading_turnstile;
pthread_mutex_t passenger_coordination_lock;
pthread_cond_t cond_car_is_loading;
volatile sig_atomic_t shutdown_flag = 0;
volatile int loading_car_id = -1;
int g_num_cars = 0;
Car* cars = NULL;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        if (shutdown_flag) { // Force exit on second Ctrl+C
            fprintf(stderr, "\nForcing shutdown...\n");
            exit(1);
        }
        shutdown_flag = 1;

        // Wake up ALL potentially sleeping threads
        ordered_mutex_broadcast(&ticket_queue_lock);
        ordered_mutex_broadcast(&unloading_turnstile);
        pthread_cond_broadcast(&cond_car_is_loading);

        if (cars != NULL) {
            for (int i = 1; i <= g_num_cars; ++i) {
                 pthread_cond_broadcast(&cars[i].cond_passenger_unboarded);
                 pthread_cond_broadcast(&cars[i].cond_passenger_boarded);
            }
        }
    }
}

void get_time(char* buffer, size_t buf_size) {
    time_t current_time = time(NULL);
    int elapsed = current_time - start_time;
    snprintf(buffer, buf_size, "%02d:%02d:%02d", elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
}

void print_help() {
    printf("USAGE: ./park [OPTIONS]\n");
    printf("  -n, INT    Number of passenger threads\n");
    printf("  -c, INT    Number of cars\n");
    printf("  -p, INT    Capacity per car\n");
    printf("  -w, INT    Car waiting period in seconds\n");
    printf("  -r, INT    Ride duration in seconds\n");
    printf("  -h, --help Display this help message\n");
}

int main(int argc, char* argv[]) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int num_passengers = 8, car_capacity = 4, ride_duration = 5, wait_period = 10;
    g_num_cars = 3;

    int opt;
    while ((opt = getopt(argc, argv, "n:c:p:r:w:h")) != -1) {
        switch (opt) {
            case 'n': num_passengers = atoi(optarg); break;
            case 'c': g_num_cars = atoi(optarg); break;
            case 'p': car_capacity = atoi(optarg); break;
            case 'r': ride_duration = atoi(optarg); break;
            case 'w': wait_period = atoi(optarg); break;
            case 'h': print_help(); return 0;
            default: return 1;
        }
    }

    start_time = time(NULL);
    srand(start_time);

    ordered_mutex_init(&ticket_queue_lock);
    pthread_mutex_init(&loading_bay_lock, NULL);
    ordered_mutex_init(&unloading_turnstile);
    pthread_mutex_init(&passenger_coordination_lock, NULL);
    pthread_cond_init(&cond_car_is_loading, NULL);

    cars = malloc(sizeof(Car) * (g_num_cars + 1));
    for (int i = 1; i <= g_num_cars; i++) {
        cars[i].car_id = i;
        cars[i].passengers_on_board = 0;
        pthread_mutex_init(&cars[i].lock, NULL);
        pthread_cond_init(&cars[i].cond_passenger_boarded, NULL);
        pthread_cond_init(&cars[i].cond_passenger_unboarded, NULL);
    }
    
    printf("===== DUCK PARK SIMULATION (Part 2) =====\n");
    printf("  - Number of passenger threads: %d\n", num_passengers);
    printf("  - Number of cars: %d\n", g_num_cars);
    printf("  - Capacity per car: %d\n", car_capacity);
    printf("  - Car waiting period: %d seconds\n", wait_period);
    printf("  - Ride duration: %d seconds\n\n", ride_duration);
    fflush(stdout);

    pthread_t p_threads[num_passengers];
    pthread_t c_threads[g_num_cars];
    
    for (int i = 0; i < num_passengers; i++) {
        passenger_info* p_info = malloc(sizeof(passenger_info));
        p_info->passenger_num = i + 1;
        p_info->car_capacity = car_capacity;
        pthread_create(&p_threads[i], NULL, passenger_func, p_info);
    }
    for (int i = 0; i < g_num_cars; i++) {
        car_info* c_info = malloc(sizeof(car_info));
        c_info->car_id = i + 1;
        c_info->capacity = car_capacity;
        c_info->ride_duration = ride_duration;
        c_info->wait_period = wait_period;
        pthread_create(&c_threads[i], NULL, car_func, c_info);
    }
    
    for (int i = 0; i < num_passengers; i++) {
        pthread_join(p_threads[i], NULL);
    }
    for (int i = 0; i < g_num_cars; i++) {
        pthread_join(c_threads[i], NULL);
    }
    
    printf("All threads have exited. Cleaning up resources.\n");
    pthread_mutex_destroy(&loading_bay_lock);
    ordered_mutex_destroy(&unloading_turnstile);
    ordered_mutex_destroy(&ticket_queue_lock);
    pthread_mutex_destroy(&passenger_coordination_lock);
    pthread_cond_destroy(&cond_car_is_loading);
    for (int i = 1; i <= g_num_cars; i++) {
        pthread_mutex_destroy(&cars[i].lock);
        pthread_cond_destroy(&cars[i].cond_passenger_boarded);
        pthread_cond_destroy(&cars[i].cond_passenger_unboarded);
    }
    free(cars);

    printf("Cleanup complete.\n");
    return 0;
}