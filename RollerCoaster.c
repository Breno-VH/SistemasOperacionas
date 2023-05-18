/*
anotacoes:
-link do codigo original: https://github.com/sp0oks/multithread-drifting
TODO
-colocar um mutex no print para nao ter problemas na visualizacao

*/

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// Maximum number of passenger threads available
#define MAX_PASSENGERS 12
#define MAX_RIDES 3
#define MAX_CARS 5
#define MAX_CAPACITY 9


/* Variables */
pthread_mutex_t check_in_lock; // mutex to control access of the 'boarded' variable
pthread_mutex_t riding_lock; // mutex to control access of the 'unboarded' variable

sem_t board_queue; // semaphore to ensure boarding of C passenger threads
sem_t all_boarded; // binary semaphore to tell passenger threads to wait for the next ride
sem_t unboard_queue; // semaphore to ensure unboarding of C passenger threads
sem_t all_unboarded; // binary semaphore to signal passenger threads for boarding
sem_t loading_area[MAX_CARS];
sem_t unloading_area[MAX_CARS];
sem_t killRollerCoaster;


pthread_t passenger[MAX_PASSENGERS]; //array for passenger threads
int ridesTaken[MAX_PASSENGERS]; //rides taken per passenger

volatile int boarded = 0; // current number of passenger threads that have boarded
volatile int unboarded = 0; // current number of passenger threads that have unboarded
volatile int current_ride = 0; // current number of rides
volatile int total_rides; // total number of coaster rides for the instance
volatile int passengers; // current number of passenger threads
volatile int capacity; // current capacity of the car thread
volatile int total_cars;


//arrays for ascii visualization
char* line0;
char* line0Busy;
char* line1;
char* line2;

/* Helper functions */
int min(int a, int b) {
	return (a<b? a:b);
}


int next(int pos) {
	return (1 + pos) % total_cars;
}

void load(int id, int run) {
	printf("Ride #%d of car %d wil begin, time to load!\n", run+1, id+1);
	printf("This car's capacity is %d\n", capacity);
	sleep(2);
}
void run(int id) {
	printf("The car %d is full, time to ride!\n", id+1);
	sleep(2);
	printf("The car %d is now riding the roller coaster!\n", id+1);
	sleep(3);
}
void unload(int id) {
	printf("The ride is over, time to unload car %d\n", id+1);
	sleep(2);
}
void board() {
	printf("%d passengers have boarded the car\n", boarded);
	cartImage(boarded);
	
	sleep(2);
}

void unboard() {
	printf("%d passengers have unboarded the car\n", unboarded);
	int ocupiedCars = min(capacity, passengers) - unboarded;
	cartImage(ocupiedCars);
	
	sleep(2);
}

void cartImage(int filledPosition) {
	for (int i = 0; i < capacity; i++) {
		if (i < filledPosition) printf("%s", line0Busy);
		else printf("%s", line0);
    }
    printf("\n");
    for (int i = 0; i < capacity; i++) {
        printf("%s", line1);
    }
    printf("\n");
	for (int i = 0; i < capacity; i++) {
        printf("%s", line2);
    }
	printf("\n");
}
/* end helper functions*/


/* Thread Functions */
void* carThread(void* id) {
	int i;
	int myRun = 0;
	int carId = (int) id;
	int limitQueue = min(capacity, passengers);
	// Run the roller coaster for <total_rides> times
	while(myRun < total_rides) {
		sem_wait(&loading_area[carId]); //waits until its their turn to load
		load(carId, myRun);
		
		for(i = 0; i < limitQueue; i++) sem_post(&board_queue); // Signal C passenger threads to board the car

		sem_wait(&all_boarded); // Wait for all passengers to board
		
		sem_post(&loading_area[next(carId)]); //next area can begin boarding
		
		run(carId);

		sem_wait(&unloading_area[carId]); //wait until its their turn to unload
		unload(carId);
		
		for(i = 0; i < limitQueue; i++) sem_post(&unboard_queue); // Signal the passengers in the car to unboard
		sem_wait(&all_unboarded); // Tell the queue to start boarding again
		printf("The car is now empty!\n\n");
		sem_post(&unloading_area[next(carId)]); //next area can begin unboading
		
		myRun++;
	}

	//last car in last ride sends kill signal to passenger threads
	if (carId == ((int)total_rides -1)) sem_post(&killRollerCoaster);

	return NULL;
}

/*
	runs forever, until a kill signal is sent through another thread
*/
void* passengerThread(void* id) {
	int passengerId = (int) id;
	while(1) {
		sem_wait(&board_queue); // Wait for the car thread signal to board
		
		pthread_mutex_lock(&check_in_lock); // Lock access to shared variable before incrementing
		boarded++;
		board();
		ridesTaken[passengerId] += 1;
		if (boarded == capacity || boarded == passengers) {
			sem_post(&all_boarded); // If this is the last passenger to board, signal the car to run
			boarded = 0;
		}
		pthread_mutex_unlock(&check_in_lock); // Unlock access to shared variable

		sem_wait(&unboard_queue); // Wait for the ride to end
	
		pthread_mutex_lock(&riding_lock); // Lock access to shared variable before incrementing
		unboarded++;
		unboard();
		if (unboarded == capacity || unboarded == passengers) {
			sem_post(&all_unboarded); // If this is the last passenger to unboard, signal the car to allow boarding
			unboarded = 0;
		}
		pthread_mutex_unlock(&riding_lock); // Unlock access to shared variable
	}
    	return NULL;
}

/*
 thread that kills remaining passengers
 awaiting for their ride once all rides have finished
*/
void* executioner() {
	int i = 0;
	sem_wait(&killRollerCoaster);
	for (i = 0; i < passengers; i++) {
		pthread_join(passenger[i], NULL);
	}
	return NULL;
}
/* end thread functions */

/* Main program */
int main() {
	// Set new instance of passenger threads, car capacity and total rides values
	srand(time(NULL));
	passengers = 2 + rand() % (MAX_PASSENGERS-2);
	capacity = 1 + rand() % (MAX_CAPACITY - 1);
	total_rides = 1 + rand() % (MAX_RIDES-1);
	total_cars = 2 + rand() % (MAX_CARS - 2);

	//set drawing lines of the car
	line0 = "     ____/   ___   ";
	line0Busy = "     ____/ O ___   ";
	line1 = "  --|_   \\__'  _\\--";
	line2 = "     --(*)--(*)--  ";


	pthread_t executioner; //thread que mata as threads passageiras
	pthread_t car[total_cars]; //array de carros
	int i;
	
	for (i = 0; i < passengers; i++) {
		ridesTaken[i] = 0;
	}


	// Create new mutexes and semaphores
	pthread_mutex_init(&check_in_lock, NULL);
	pthread_mutex_init(&riding_lock, NULL);

	for (i = 0; i < total_cars; i++) {
		sem_init(&loading_area[i], 0, 0);
		sem_init(&unloading_area[i], 0, 0);
	} 
	sem_init(&executioner, 0, 0);
	sem_init(&board_queue, 0, 0);
	sem_init(&all_boarded, 0, 0);
	sem_init(&unboard_queue, 0, 0);
	sem_init(&all_unboarded, 0, 0);

	printf("Today the roller coaster will ride %d times!\n", total_rides);
	printf("There are %d passengers waiting in the roller coaster queue!\n\n", passengers);
	printf("There are %d cars waiting in the roller coaster queue!\n\n", total_cars);
	
	// Create the threads and start the roller coaster

	sem_post(&loading_area[0]);
	sem_post(&unloading_area[0]);
	for (i = 0; i < total_cars; i++) pthread_create(&car[i], NULL, carThread, (void*) i);
	for(i = 0; i < passengers; i++) pthread_create(&passenger[i], NULL, passengerThread, (void*) i);
	// Join the car thread when all rides have been completed
	for (i = 0; i < total_cars; i++) pthread_join(car[i], NULL);
	pthread_join(executioner, NULL);
	
	printf("That's all rides for today, the roller coaster is shutting down.\n");
	for (i = 0; i < passengers; i++) {
		printf("Passgenger %d has taken %d ride(s)\n", i+1, ridesTaken[i]);
	}


	// Destroy mutexes and semaphores
	pthread_mutex_destroy(&check_in_lock);
	pthread_mutex_destroy(&riding_lock);
	sem_destroy(&board_queue);
	sem_destroy(&all_boarded);
	sem_destroy(&unboard_queue);
	sem_destroy(&all_unboarded);
	for (i = 0; i < total_cars; i++) {
		sem_destroy(&loading_area[i]);
		sem_destroy(&unloading_area[i]);
	} 

	return 0;
}
