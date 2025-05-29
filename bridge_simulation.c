#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Total number of farmers from each direction
#define NUM_FARMERS_NORTH 5
#define NUM_FARMERS_SOUTH 5
#define MAX_CROSSING_TIME 3 // Maximum bridge crossing time in seconds

// Possible directions
typedef enum {
    NONE,
    NORTHBOUND,
    SOUTHBOUND
} Direction;

// Shared variables for the bridge
pthread_mutex_t bridge_mutex;
pthread_cond_t northbound_cv;
pthread_cond_t southbound_cv;
int farmers_on_bridge = 0;
Direction current_direction_on_bridge = NONE;
int northbound_waiting_count = 0;
int southbound_waiting_count = 0;

// Next ID for farmers
int next_farmer_id = 0;
pthread_mutex_t id_mutex; // For thread-safe ID assignment

// Function to get a unique ID for each farmer
int get_farmer_id() {
    pthread_mutex_lock(&id_mutex);
    int id = ++next_farmer_id;
    pthread_mutex_unlock(&id_mutex);
    return id;
}

// Function for northbound farmers
void *northbound_farmer(void *arg) {
    int farmer_id = get_farmer_id();
    int crossing_time = (rand() % MAX_CROSSING_TIME) + 1;

    printf("Northbound Farmer %d arrived at the bridge.\n", farmer_id);

    // Request to enter the bridge
    pthread_mutex_lock(&bridge_mutex);
    northbound_waiting_count++;
    while (farmers_on_bridge > 0 && current_direction_on_bridge == SOUTHBOUND) {
        // If southbound farmers are on the bridge, wait
        printf("Northbound Farmer %d is waiting (Current Direction: Southbound, On Bridge: %d).\n", farmer_id, farmers_on_bridge);
        pthread_cond_wait(&northbound_cv, &bridge_mutex);
    }
    // Fairness: If the bridge was southbound and southbound farmers are waiting, let them go first.
    while(current_direction_on_bridge == SOUTHBOUND && southbound_waiting_count > 0){
         printf("Northbound Farmer %d is waiting for waiting Southbound farmers to cross.\n", farmer_id);
         pthread_cond_signal(&southbound_cv); // Signal one southbound farmer
         pthread_cond_wait(&northbound_cv, &bridge_mutex); // Wait again
    }

    northbound_waiting_count--;
    farmers_on_bridge++;
    current_direction_on_bridge = NORTHBOUND;
    printf("Northbound Farmer %d entered the bridge. (On Bridge: %d)\n", farmer_id, farmers_on_bridge);
    pthread_mutex_unlock(&bridge_mutex);

    // Crossing the bridge
    printf("Northbound Farmer %d is crossing the bridge (%d seconds)...\n", farmer_id, crossing_time);
    sleep(crossing_time);

    // Exiting the bridge
    pthread_mutex_lock(&bridge_mutex);
    farmers_on_bridge--;
    printf("Northbound Farmer %d exited the bridge. (On Bridge: %d)\n", farmer_id, farmers_on_bridge);

    if (farmers_on_bridge == 0) {
        printf("Bridge is now empty from Northbound direction.\n");
        current_direction_on_bridge = NONE;
        // Prioritize waiting southbound farmers for fairness
        if (southbound_waiting_count > 0) {
            printf("Signaling waiting Southbound farmers.\n");
            pthread_cond_broadcast(&southbound_cv);
        } else if (northbound_waiting_count > 0) { // If no southbound waiting, signal other northbound
            printf("Signaling waiting Northbound farmers.\n");
            pthread_cond_broadcast(&northbound_cv);
        }
    } else {
        // If still northbound farmers on bridge, signal other waiting northbound farmers
         if (northbound_waiting_count > 0) {
            pthread_cond_signal(&northbound_cv);
        }
    }
    pthread_mutex_unlock(&bridge_mutex);

    pthread_exit(NULL);
}

// Function for southbound farmers
void *southbound_farmer(void *arg) {
    int farmer_id = get_farmer_id();
    int crossing_time = (rand() % MAX_CROSSING_TIME) + 1;

    printf("Southbound Farmer %d arrived at the bridge.\n", farmer_id);

    // Request to enter the bridge
    pthread_mutex_lock(&bridge_mutex);
    southbound_waiting_count++;
    while (farmers_on_bridge > 0 && current_direction_on_bridge == NORTHBOUND) {
        // If northbound farmers are on the bridge, wait
        printf("Southbound Farmer %d is waiting (Current Direction: Northbound, On Bridge: %d).\n", farmer_id, farmers_on_bridge);
        pthread_cond_wait(&southbound_cv, &bridge_mutex);
    }
    // Fairness: If the bridge was northbound and northbound farmers are waiting, let them go first.
    while(current_direction_on_bridge == NORTHBOUND && northbound_waiting_count > 0){
         printf("Southbound Farmer %d is waiting for waiting Northbound farmers to cross.\n", farmer_id);
         pthread_cond_signal(&northbound_cv); // Signal one northbound farmer
         pthread_cond_wait(&southbound_cv, &bridge_mutex); // Wait again
    }
    southbound_waiting_count--;
    farmers_on_bridge++;
    current_direction_on_bridge = SOUTHBOUND;
    printf("Southbound Farmer %d entered the bridge. (On Bridge: %d)\n", farmer_id, farmers_on_bridge);
    pthread_mutex_unlock(&bridge_mutex);

    // Crossing the bridge
    printf("Southbound Farmer %d is crossing the bridge (%d seconds)...\n", farmer_id, crossing_time);
    sleep(crossing_time);

    // Exiting the bridge
    pthread_mutex_lock(&bridge_mutex);
    farmers_on_bridge--;
    printf("Southbound Farmer %d exited the bridge. (On Bridge: %d)\n", farmer_id, farmers_on_bridge);

    if (farmers_on_bridge == 0) {
        printf("Bridge is now empty from Southbound direction.\n");
        current_direction_on_bridge = NONE;
        // Prioritize waiting northbound farmers for fairness
        if (northbound_waiting_count > 0) {
            printf("Signaling waiting Northbound farmers.\n");
            pthread_cond_broadcast(&northbound_cv);
        } else if (southbound_waiting_count > 0) { // If no northbound waiting, signal other southbound
             printf("Signaling waiting Southbound farmers.\n");
            pthread_cond_broadcast(&southbound_cv);
        }
    } else {
        // If still southbound farmers on bridge, signal other waiting southbound farmers
        if (southbound_waiting_count > 0) {
            pthread_cond_signal(&southbound_cv);
        }
    }
    pthread_mutex_unlock(&bridge_mutex);

    pthread_exit(NULL);
}

int main() {
    pthread_t north_threads[NUM_FARMERS_NORTH];
    pthread_t south_threads[NUM_FARMERS_SOUTH];

    // Initialize mutexes and condition variables
    pthread_mutex_init(&bridge_mutex, NULL);
    pthread_mutex_init(&id_mutex, NULL);
    pthread_cond_init(&northbound_cv, NULL);
    pthread_cond_init(&southbound_cv, NULL);

    // Initialize random number generator
    srand(time(NULL));

    printf("Single-Lane Bridge Simulation Started.\n");
    printf("-------------------------------------\n");

    // Create threads for northbound farmers
    for (int i = 0; i < NUM_FARMERS_NORTH; i++) {
        if (pthread_create(&north_threads[i], NULL, northbound_farmer, NULL) != 0) {
            perror("Error creating northbound thread");
            return 1;
        }
        // Small delay to stagger farmer arrivals slightly (optional)
        // usleep((rand() % 500) * 1000); // 0 to 0.5 seconds
    }

    // Create threads for southbound farmers
    for (int i = 0; i < NUM_FARMERS_SOUTH; i++) {
        if (pthread_create(&south_threads[i], NULL, southbound_farmer, NULL) != 0) {
            perror("Error creating southbound thread");
            return 1;
        }
        // Small delay to stagger farmer arrivals slightly (optional)
        // usleep((rand() % 500) * 1000); // 0 to 0.5 seconds
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_FARMERS_NORTH; i++) {
        pthread_join(north_threads[i], NULL);
    }
    for (int i = 0; i < NUM_FARMERS_SOUTH; i++) {
        pthread_join(south_threads[i], NULL);
    }

    // Destroy mutexes and condition variables
    pthread_mutex_destroy(&bridge_mutex);
    pthread_mutex_destroy(&id_mutex);
    pthread_cond_destroy(&northbound_cv);
    pthread_cond_destroy(&southbound_cv);

    printf("-------------------------------------\n");
    printf("Single-Lane Bridge Simulation Finished.\n");

    return 0;
}