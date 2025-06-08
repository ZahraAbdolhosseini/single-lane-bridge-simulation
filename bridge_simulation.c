#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_FARMERS_NORTH 5
#define NUM_FARMERS_SOUTH 5
#define MAX_CROSSING_TIME 3

typedef enum {
    NONE,
    NORTHBOUND,
    SOUTHBOUND
} Direction;

pthread_mutex_t bridge_mutex;
pthread_cond_t northbound_cv;
pthread_cond_t southbound_cv;
int farmers_on_bridge = 0;
Direction current_direction_on_bridge = NONE;
int northbound_waiting_count = 0;
int southbound_waiting_count = 0;

int next_farmer_id = 0;
pthread_mutex_t id_mutex;

int get_farmer_id() {
    pthread_mutex_lock(&id_mutex);
    int id = ++next_farmer_id;
    pthread_mutex_unlock(&id_mutex);
    return id;
}

void *northbound_farmer(void *arg) {
    int farmer_id = get_farmer_id();
    int crossing_time = (rand() % MAX_CROSSING_TIME) + 1;

    printf("Northbound Farmer %d arrived at the bridge.\n", farmer_id);

    pthread_mutex_lock(&bridge_mutex);
    northbound_waiting_count++;
    while (farmers_on_bridge > 0 && current_direction_on_bridge == SOUTHBOUND) {
        printf("Northbound Farmer %d is waiting (Current Direction: Southbound, On Bridge: %d).\n", farmer_id, farmers_on_bridge);
        pthread_cond_wait(&northbound_cv, &bridge_mutex);
    }
    while (current_direction_on_bridge == SOUTHBOUND && southbound_waiting_count > 0) {
        printf("Northbound Farmer %d is waiting for waiting Southbound farmers to cross.\n", farmer_id);
        pthread_cond_signal(&southbound_cv);
        pthread_cond_wait(&northbound_cv, &bridge_mutex);
    }

    northbound_waiting_count--;
    farmers_on_bridge++;
    current_direction_on_bridge = NORTHBOUND;
    printf("Northbound Farmer %d entered the bridge. (On Bridge: %d)\n", farmer_id, farmers_on_bridge);
    pthread_mutex_unlock(&bridge_mutex);

    printf("Northbound Farmer %d is crossing the bridge (%d seconds)...\n", farmer_id, crossing_time);
    sleep(crossing_time);

    pthread_mutex_lock(&bridge_mutex);
    farmers_on_bridge--;
    printf("Northbound Farmer %d exited the bridge. (On Bridge: %d)\n", farmer_id, farmers_on_bridge);
    if (farmers_on_bridge == 0) {
        printf("Bridge is now empty from Northbound direction.\n");
        current_direction_on_bridge = NONE;
        if (southbound_waiting_count > 0) {
            printf("Signaling waiting Southbound farmers.\n");
            pthread_cond_broadcast(&southbound_cv);
        } else if (northbound_waiting_count > 0) {
            printf("Signaling waiting Northbound farmers.\n");
            pthread_cond_broadcast(&northbound_cv);
        }
    } else {
        if (northbound_waiting_count > 0) {
            pthread_cond_signal(&northbound_cv);
        }
    }
    pthread_mutex_unlock(&bridge_mutex);

    pthread_exit(NULL);
}

void *southbound_farmer(void *arg) {
    int farmer_id = get_farmer_id();
    int crossing_time = (rand() % MAX_CROSSING_TIME) + 1;

    printf("Southbound Farmer %d arrived at the bridge.\n", farmer_id);

    pthread_mutex_lock(&bridge_mutex);
    southbound_waiting_count++;
    while (farmers_on_bridge > 0 && current_direction_on_bridge == NORTHBOUND) {
        printf("Southbound Farmer %d is waiting (Current Direction: Northbound, On Bridge: %d).\n", farmer_id, farmers_on_bridge);
        pthread_cond_wait(&southbound_cv, &bridge_mutex);
    }
    while (current_direction_on_bridge == NORTHBOUND && northbound_waiting_count > 0) {
        printf("Southbound Farmer %d is waiting for waiting Northbound farmers to cross.\n", farmer_id);
        pthread_cond_signal(&northbound_cv);
        pthread_cond_wait(&southbound_cv, &bridge_mutex);
    }

    southbound_waiting_count--;
    farmers_on_bridge++;
    current_direction_on_bridge = SOUTHBOUND;
    printf("Southbound Farmer %d entered the bridge. (On Bridge: %d)\n", farmer_id, farmers_on_bridge);
    pthread_mutex_unlock(&bridge_mutex);

    printf("Southbound Farmer %d is crossing the bridge (%d seconds)...\n", farmer_id, crossing_time);
    sleep(crossing_time);

    pthread_mutex_lock(&bridge_mutex);
    farmers_on_bridge--;
    printf("Southbound Farmer %d exited the bridge. (On Bridge: %d)\n", farmer_id, farmers_on_bridge);
    if (farmers_on_bridge == 0) {
        printf("Bridge is now empty from Southbound direction.\n");
        current_direction_on_bridge = NONE;
        if (northbound_waiting_count > 0) {
            printf("Signaling waiting Northbound farmers.\n");
            pthread_cond_broadcast(&northbound_cv);
        } else if (southbound_waiting_count > 0) {
            printf("Signaling waiting Southbound farmers.\n");
            pthread_cond_broadcast(&southbound_cv);
        }
    } else {
        if (southbound_waiting_count > 0) {
            pthread_cond_signal(&southbound_cv);
        }
    }
    pthread_mutex_unlock(&bridge_mutex);

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_FARMERS_NORTH + NUM_FARMERS_SOUTH];

    pthread_mutex_init(&bridge_mutex, NULL);
    pthread_mutex_init(&id_mutex, NULL);
    pthread_cond_init(&northbound_cv, NULL);
    pthread_cond_init(&southbound_cv, NULL);

    srand(time(NULL));

    printf("Single-Lane Bridge Simulation (Random Thread Creation Order)\n");
    printf("------------------------------------------------------------\n");

    int created_north = 0, created_south = 0, total = 0;
    while (created_north < NUM_FARMERS_NORTH || created_south < NUM_FARMERS_SOUTH) {
        int pick = rand() % 2; // 0 = north, 1 = south

        if ((pick == 0 && created_north < NUM_FARMERS_NORTH) || created_south >= NUM_FARMERS_SOUTH) {
            pthread_create(&threads[total++], NULL, northbound_farmer, NULL);
            created_north++;
        } else {
            pthread_create(&threads[total++], NULL, southbound_farmer, NULL);
            created_south++;
        }

        usleep((rand() % 500) * 1000); // 0 to 0.5 second delay
    }

    for (int i = 0; i < total; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&bridge_mutex);
    pthread_mutex_destroy(&id_mutex);
    pthread_cond_destroy(&northbound_cv);
    pthread_cond_destroy(&southbound_cv);

    printf("------------------------------------------------------------\n");
    printf("Single-Lane Bridge Simulation Finished.\n");

    return 0;
}
