#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include "threads.h"

// --- Global Variable Definitions ---
time_t start_time;
Shared_State* shared_state;
ordered_mutex_t ticket_queue_lock;
pthread_mutex_t loading_bay_lock;
ordered_mutex_t unloading_turnstile;
pthread_mutex_t passenger_coordination_lock;
pthread_cond_t cond_car_is_loading;
volatile sig_atomic_t shutdown_flag = 0;
volatile int loading_car_id = -1;
int g_num_cars = 0;
Car* cars = NULL;

// --- Function Prototypes ---
const char* get_status_str(CarStatus status); // <-- ADDED PROTOTYPE TO FIX WARNING
void signal_handler(int signum);

// --- Signal Handler for Graceful Shutdown ---
void signal_handler(int signum) {
    if (signum == SIGINT) {
        if (shutdown_flag) {
            fprintf(stderr, "\nForcing exit...\n");
            exit(1);
        }
        printf("\nCtrl+C detected. Shutting down simulation gracefully...\n");
        shutdown_flag = 1;

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

// --- Utility Functions ---
const char* get_status_str(CarStatus status) {
    switch (status) {
        case WAITING: return "WAITING";
        case LOADING: return "LOADING";
        case RUNNING: return "RUNNING";
        case UNLOADING: return "UNLOADING";
        default: return "UNKNOWN";
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

void monitor_func(int num_cars, int capacity) {
    char time_buffer[TIME_STR_BUF_SIZE];
    while (1) {
        if(shutdown_flag) break;
        sleep(5);
        if(shutdown_flag) break;

        get_time(time_buffer, sizeof(time_buffer));

        pthread_mutex_lock(&shared_state->lock);
        printf("\n\n===== [Monitor] System State at %s =====\n", time_buffer);
        printf("Ticket Queue: "); print_queue(&shared_state->ticket_queue);
        printf("\nRide Queue:   "); print_queue(&shared_state->ride_queue);
        printf("\n");

        for (int i = 1; i <= num_cars; i++) {
            printf("Car %d Status: %-10s (%d/%d passengers)\n", i,
                   get_status_str(shared_state->car_statuses[i]),
                   shared_state->passengers_in_car[i], capacity);
        }
        
        int passengers_in_queues = shared_state->ticket_queue.count + shared_state->ride_queue.count;
        int passengers_on_rides = 0;
        for(int i = 1; i <= num_cars; i++) {
            passengers_on_rides += shared_state->passengers_in_car[i];
        }
        printf("Passengers in park: %d (exploring: %d, in queues: %d, on rides: %d)\n",
               shared_state->total_passengers_in_park,
               shared_state->passengers_exploring,
               passengers_in_queues,
               passengers_on_rides);
        
        printf("==============================================\n\n");
        fflush(stdout);
        pthread_mutex_unlock(&shared_state->lock);
    }
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

    shared_state = mmap(NULL, sizeof(Shared_State), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_state == MAP_FAILED) { perror("mmap failed"); exit(1); }

    pthread_mutexattr_t pshared_attr;
    pthread_mutexattr_init(&pshared_attr);
    pthread_mutexattr_setpshared(&pshared_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_state->lock, &pshared_attr);
    queue_init(&shared_state->ticket_queue);
    queue_init(&shared_state->ride_queue);
    shared_state->total_passengers_in_park = num_passengers;
    shared_state->passengers_exploring = 0;

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
        shared_state->car_statuses[i] = WAITING;
        shared_state->passengers_in_car[i] = 0;
    }
    
    printf("===== DUCK PARK SIMULATION =====\n");
    printf("  - Number of passenger threads: %d\n", num_passengers);
    printf("  - Number of cars: %d\n", g_num_cars);
    printf("  - Capacity per car: %d\n", car_capacity);
    printf("  - Park exploration time: 2-5 seconds\n");
    printf("  - Car waiting period: %d seconds\n", wait_period);
    printf("  - Ride duration: %d seconds\n\n", ride_duration);
    fflush(stdout);

    pid_t pid = fork();
    if (pid == -1) { perror("fork failed"); exit(1); }
    if (pid == 0) { monitor_func(g_num_cars, car_capacity); exit(0); }

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
    
    printf("\nSimulation running... Press Ctrl+C to exit.\n");
    for (int i = 0; i < num_passengers; i++) {
        pthread_join(p_threads[i], NULL);
    }
    for (int i = 0; i < g_num_cars; i++) {
        pthread_join(c_threads[i], NULL);
    }
    
    kill(pid, SIGKILL);
    wait(NULL);
    
    printf("All threads have exited. Cleaning up resources.\n");
    pthread_mutexattr_destroy(&pshared_attr);
    pthread_mutex_destroy(&shared_state->lock);
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
    munmap(shared_state, sizeof(Shared_State));

    printf("Cleanup complete.\n");
    return 0;
}