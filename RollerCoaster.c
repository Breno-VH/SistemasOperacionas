/*
anotacoes:
-link do codigo original: https://github.com/sp0oks/multithread-drifting

-ajeitar os prints

-consertar o current ride

-colocar currentBoardingCar currentUnboardingCar

-programa nao termina direito por conta da loading area e unloading
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// Maximum number of passenger threads available
#define MAX_PASSENGERS 25
#define MAX_RIDES 3
#define MAX_CARS 10


/* Variables */
pthread_mutex_t check_in_lock; // mutex to control access of the 'boarded' variable
pthread_mutex_t riding_lock; // mutex to control access of the 'unboarded' variable

sem_t board_queue; // semaphore to ensure boarding of C passenger threads
sem_t all_boarded; // binary semaphore to tell passenger threads to wait for the next ride
sem_t unboard_queue; // semaphore to ensure unboarding of C passenger threads
sem_t all_unboarded; // binary semaphore to signal passenger threads for boarding
sem_t loading_area[MAX_CARS];
sem_t unloading_area[MAX_CARS];


volatile int boarded = 0; // current number of passenger threads that have boarded
volatile int unboarded = 0; // current number of passenger threads that have unboarded
volatile int current_ride = 0; // current number of rides
volatile int total_rides; // total number of coaster rides for the instance
volatile int passengers; // current number of passenger threads
volatile int capacity; // current capacity of the car thread
volatile int total_cars;


int currentBoardingCar;
int currentUnboardingCar;

char* line0;
char* line0Busy;
char* line1;
char* line2;

/* Helper functions */
int next(int pos) {
	return (1 + pos) % total_cars;
}

void load(int id){
	printf("Ride #%d will begin, time to load car %d!\n", current_ride+1, id+1);
	printf("This car's capacity is %d\n", capacity);
	sleep(2);
}
void run(int id){
	printf("The car %d is full, time to ride!\n", id+1);
	sleep(2);
	printf("The car %d is now riding the roller coaster!\n", id+1);
	sleep(5);
}
void unload(int id){
	printf("The ride is over, time to unload car %d\n", id+1);
	sleep(2);
}
void board(){
	printf("%d passengers have boarded the car\n", boarded);
	int freeCars = capacity - boarded;
	for (int i = 0; i < capacity; i++) {
		if (i < boarded) printf("%s", line0Busy);
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
	sleep(rand()%2);
}

void unboard(){
	printf("%d passengers have unboarded the car\n", unboarded);
	int ocupiedCars = capacity - unboarded;
	for (int i = 0; i < capacity; i++) {
		if (i < ocupiedCars) printf("%s", line0Busy);
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
	sleep(rand()%2);
}

/* Thread Functions */
void* carThread(void* id){
	int i;
	int carId = (int) id;
	// Run the roller coaster for <total_rides> times
	while(current_ride < total_rides){
		sem_wait(&loading_area[carId]);
		load(id);
		
		for(i = 0; i < capacity; i++) sem_post(&board_queue); // Signal C passenger threads to board the car
		sem_wait(&all_boarded); // Wait for all passengers to board
		
		sem_post(&loading_area[next(carId)]);
		
		run(id);

		sem_wait(&unloading_area[carId]);
		unload(id);
		
		for(i = 0; i < capacity; i++) sem_post(&unboard_queue); // Signal the passengers in the car to unboard
		sem_wait(&all_unboarded); // Tell the queue to start boarding again
		printf("The car is now empty!\n\n");
		sem_post(&unloading_area[next(carId)]);
		
		if (carId == total_cars-1) {
			current_ride++;
		}
	}
	return NULL;
}

void* passengerThread(){
	// Run indefinitely, threads will be destroyed when the main process exits
	while(1){
		sem_wait(&board_queue); // Wait for the car thread signal to board
		
		pthread_mutex_lock(&check_in_lock); // Lock access to shared variable before incrementing
		boarded++;
		board();
		if (boarded == capacity){
			sem_post(&all_boarded); // If this is the last passenger to board, signal the car to run
			boarded = 0;
		}
		pthread_mutex_unlock(&check_in_lock); // Unlock access to shared variable

		sem_wait(&unboard_queue); // Wait for the ride to end
	
		pthread_mutex_lock(&riding_lock); // Lock access to shared variable before incrementing
		unboarded++;
		unboard();
		if (unboarded == capacity){
			sem_post(&all_unboarded); // If this is the last passenger to unboard, signal the car to allow boarding
			unboarded = 0;
		}
		pthread_mutex_unlock(&riding_lock); // Unlock access to shared variable
	}
    	return NULL;
}

/* Main program */
int main() {
	// Set new instance of passenger threads, car capacity and total rides values
	srand(time(NULL));
	passengers = 2 + rand() % MAX_PASSENGERS;
	capacity = 1 + rand() % (passengers - 1);
	total_rides = 1 + rand() % MAX_RIDES;
	total_cars = 2 + rand() % (MAX_CARS - 2);

	//set drawing lines
	line0 = "     ____/   ___   ";
	line0Busy = "     ____/ O ___   ";
	line1 = "  --|_   \\__'  _\\--";
	line2 = "     --(*)——(*)--  ";


	pthread_t passenger[passengers];
	pthread_t car[total_cars];
	int i;
	
	// Create new mutexes and semaphores
	pthread_mutex_init(&check_in_lock, NULL);
	pthread_mutex_init(&riding_lock, NULL);

	for (i = 0; i < total_cars; i++) {
		sem_init(&loading_area[i], 0, 0);
		sem_init(&unloading_area[i], 0, 0);
	} 

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
	for(i = 0; i < passengers; i++) pthread_create(&passenger[i], NULL, passengerThread, NULL);
	// Join the car thread when all rides have been completed
	pthread_join(car, NULL);
	
	printf("That's all rides for today, the roller coaster is shutting down.\n");

	// Destroy mutexes and semaphores
	pthread_mutex_destroy(&check_in_lock);
	pthread_mutex_destroy(&riding_lock);
	sem_destroy(&board_queue);
	sem_destroy(&all_boarded);
	sem_destroy(&unboard_queue);
	sem_destroy(&all_unboarded);

	return 0;
}
