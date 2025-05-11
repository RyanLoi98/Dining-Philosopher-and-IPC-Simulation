#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

/*
 * CPSC 457 Assignment 2
 * Section 2: Part 1 (Non-bonus)
 *
 * Ryan Loi
 * Lecture: 02
 * Tutorial: T10
 */

// function declarations so functions above it can use it
int testLeft(int);
int testRight(int);
void putdown(int, int);
void eat(int, int);
void wait(int);
void testBoth(int);
void think(int, int);
void displayChopstick();



// Definitions
#define NUM_ASTRONOMERS 10
#define NUM_ASYMMETRIC 3 // Number of asymmetric astronomers
#define AVG_EAT_TIME 1
#define MAX_WAIT_TIME 2
#define MAX_PICKUP_TIME 0.5  // max time for symmetric astronomers to pick up both chopsticks

// Thinking time
#define THINKING_TIME 1

// statuses for wait variable inside the philosopher_behavior structure
#define WAITING 1
#define NOT_WAITING 0

// types of philosopher
#define SYMMETRIC_PHILOSOPHER 0
#define ASYMMETRIC_PHILOSOPHER 1

// Indices for the internal array's inside of the philosopher id array
#define PHIL_NUM 0
#define PHIL_TYPE 1

// definitions for free and taken chopsticks
#define FREE 1
#define TAKEN 0

// States that a philosopher can be in. Also initialize an array of states, one for each philosopher
enum {EATING, THINKING} state[NUM_ASTRONOMERS];



// semaphores and mutexes
sem_t mutex; // to lock the monitor's critical section and ensure only one philosopher has access at a time - queuing to get in
// this mutex2 is for expedited putdown, so philosophers who are done eating don't have to wait in line for access to the critical
// region before they can putdown the chopsticks. Therefore if chopsticks are available faster, philosophers can eat faster too.
sem_t mutex2;
sem_t next; // Enforces the signal and wait protocol so there is only 1 active process in the monitor at a time.

// Counter for number of philosophers waiting to get chopsticks
int next_count = 0;

int ordering[NUM_ASTRONOMERS];  // 0 means symmetric, 1 means asymmetric

// an array to hold all of the chopsticks - 1= free, 0 = taken
int chopsticks[NUM_ASTRONOMERS];

// Monitor code and logic ----------------------------------------------------------------------------------------------

/**
 * Structure defining the "waiting behavior" of a philosopher. Essentially each philosopher will have one of these
 * structures for themselves. It will allow them to wait when there are no chopsticks available for them, and only a
 * signal from another philosopher will wake them up.
 */
typedef struct {
    // int to define if a philsopher is waiting or not
    int waitStatus;
    // semaphore to allow the philosopher to wait
    sem_t wait;
} philosopher_behavior;

// initialize an array of behaviors - one for each philosopher
philosopher_behavior behavior[NUM_ASTRONOMERS];



/**
 * Monitor that allows a philosopher specified as the argument to pick up the left and right chopsticks
 * @param Phil_ID_NUM: philosopher number
 * @param PHIL_TYP: philosopher type (symmetric or asymmetric)
 */
void pickup(int Phil_ID_NUM, int PHIL_TYP){
    // acquire the mutex lock to secure the critical section - blocking can occur here if needed (and forms a line up to get into the monitor)
    sem_wait(&mutex);

    // acquire mutex2 lock. This is the mutex lock used for philosophers when they want to skip the queue and putdown their
    // chopsticks - because having a faster putdown allows more philosophers to get their chopsticks and eat.
    sem_wait(&mutex2);

    // Check if the philosopher is symmetric or asymmetric
    // Perform actions for symmetric philosophers
    if(PHIL_TYP == SYMMETRIC_PHILOSOPHER){
        // variable to hold amount of elapsed time in seconds
        double elapsedTime = 0;

        // flag to see if we have gotten both chopsticks, 1 = true, 0 = false
        int obtained = 0;

        // Creating Timer to allow looping for 2 seconds (duration that a philosopher is allowed to attempt and pick up the chopsticks)

        // Variable to hold initial time
        time_t initialTime;
        time(&initialTime);
        // variable to hold current time
        time_t currentTime;


        // loop until wait time is over - which is 2 seconds
        do{
            // take a time measurement before we get the first chopstick
            time_t pickup_time_1;
            time(&pickup_time_1);

            // pick up right first and then left - do this by using test right then test left. Chopsticks will be automatically
            // dropped if this fails, or it is not done within a specific time frame
            if(testRight(Phil_ID_NUM) && testLeft(Phil_ID_NUM)){
                // stop timer after we got the second chopstick
                time_t pickup_time_2;
                time(&pickup_time_2);

                // calculate the pickup time
                double pickupTime = difftime(pickup_time_2, pickup_time_1);

                // if we are within the pickup time frame then set the obtained status to true (1) and break out of the loop
                if(pickupTime <= MAX_PICKUP_TIME){
                    obtained = 1;
                    break;
                }
            }

            // get current time
            time(&currentTime);
            // calculate elapsed time in seconds
            elapsedTime = difftime(currentTime, initialTime);
        } while (elapsedTime <= MAX_WAIT_TIME);

        // check if failed to obtain both chopsticks, if so wait
        if(!obtained) {
            // wait
            printf("Symmetric Philosopher #%d could not pick up any chopsticks. He is now thinking while he waits...\n\n", Phil_ID_NUM);
            wait(Phil_ID_NUM);
        }


        // perform actions for asymmetric philosopohers
    }else if(PHIL_TYP == ASYMMETRIC_PHILOSOPHER){
        // variable to hold amount of elapsed time in seconds
        double elapsedTime = 0;

        // flag to see if we have gotten both chopsticks, 1 = true, 0 = false
        int obtained = 0;

        // Creating Timer to allow looping for 2 seconds (duration that a philosopher is allowed to attempt and pick up the chopsticks)

        // Variable to hold initial time
        time_t initialTime;
        time(&initialTime);
        // variable to hold current time
        time_t currentTime;


        // pick up right first
        int right = testRight(Phil_ID_NUM);

        // wait 1 second before we can pick up the left chopstick
        sleep(1);

        // loop until wait time is over - which is 2 seconds, in this time keep trying to pick up the left chopstick
        do{
            // pick up the left chopstick
            int left = testLeft(Phil_ID_NUM);

            // check if we got both the left and right chopstick, if so set obtained to true and break out of the loop
            if(left && right){
                obtained = 1;
                break;
            }

            // get current time
            time(&currentTime);
            // calculate elapsed time in seconds
            elapsedTime = difftime(currentTime, initialTime);
        } while (elapsedTime <= MAX_WAIT_TIME);

        // check if failed to obtain both chopsticks, if so wait
        if(!obtained) {
            // wait
            printf("Asymmetric Philosopher #%d could not pick up any chopsticks. He is now thinking while he waits...\n\n", Phil_ID_NUM);
            wait(Phil_ID_NUM);
        }
    }


    // if we reach this point it is time to eat
    eat(Phil_ID_NUM, PHIL_TYP);
}


/**
 * Function that allows a philosopher specified as the argument to put down both the left and right chopsticks
 * @param Phil_ID_NUM: philosopher number
 * @param PHIL_TYP: philosopher type (symmetric or asymmetric)
 */
void putdown(int Phil_ID_NUM, int PHIL_TYP){
    // Lock the monitor's critical section with mutex2
    sem_wait(&mutex2);

    // make the current philosopher think
    // set state of philosopher to thinking
    state[Phil_ID_NUM] = THINKING;

    // take the neighboring chopsticks
    chopsticks[Phil_ID_NUM] = FREE;

    if(PHIL_TYP == SYMMETRIC_PHILOSOPHER){
        printf("Symmetric philosopher # %d has put down the left chopstick!\n", Phil_ID_NUM);
    }else if (PHIL_TYP == ASYMMETRIC_PHILOSOPHER){
        printf("Asymmetric philosopher # %d has put down the left chopstick!\n", Phil_ID_NUM);
    }

    chopsticks[(Phil_ID_NUM + NUM_ASTRONOMERS - 1) % NUM_ASTRONOMERS] = FREE;

    if(PHIL_TYP == SYMMETRIC_PHILOSOPHER){
        printf("Symmetric philosopher # %d has put down the right chopstick!\n", Phil_ID_NUM);
    }else if (PHIL_TYP == ASYMMETRIC_PHILOSOPHER){
        printf("Asymmetric philosopher # %d has put down the right chopstick!\n", Phil_ID_NUM);
    }

    // print chopstick status
    displayChopstick();

    // test left and right neighbors (use test both because it checks both neighbors of a philosopher), this allows the
    // signal to wake them up if they are eligible to eat.
    testBoth((Phil_ID_NUM + 1) % NUM_ASTRONOMERS);
    testBoth((Phil_ID_NUM + NUM_ASTRONOMERS - 1) % NUM_ASTRONOMERS);

    // unlock the monitor for other philosophers to enter
    sem_post(&mutex2);
}



/**
 * Function that puts a philosopher into waiting mode because there are no chopsticks currently available. Will free up the
 * mutex lock and semaphores so other philosophers can enter the monitor.
 * @param Phil_ID_NUM
 */
void wait(int Phil_ID_NUM){
    // enable the signal function on this philosopher since it will be waiting now
    behavior[Phil_ID_NUM].waitStatus = WAITING;


    // If other philosophers are waiting
    if (next_count > 0){
        // let the philosopher currently waiting on the next semaphore into the monitor
        sem_post(&next);
    }
    else{
        // If no philosopher is waiting, unlock the monitor mutex
        sem_post(&mutex);
        sem_post(&mutex2);
    }


    // now make the philosopher wait using its behavior structure
    sem_wait(&behavior[Phil_ID_NUM].wait);
    // if the philosopher is woken up by a signal we will disable the signal function on this philosopher
    behavior[Phil_ID_NUM].waitStatus = NOT_WAITING;
}



/**
 * Function that allows a philosopher to be woken up by its neighbors from waiting on its semaphore
 * @param Phil_ID_NUM: Philosopher to be woken up
 */
void signal(int Phil_ID_NUM){
    // if the philosopher is currently waiting
    if (behavior[Phil_ID_NUM].waitStatus == WAITING){
        // increment next count because another philosopher is waiting to get into the monitor
        next_count++;
        // Signal the waiting philosophers semaphore to wake it up
        sem_post(&behavior[Phil_ID_NUM].wait);
        // wait on next until the monitor is cleared of any other philosopher (next will be posted before a philosopher leaves)
        sem_wait(&next);
        // once it is done it can decrement next count because philosopher is now going through the monitor and is no longer waiting
        next_count--;
    }

}

// End of monitor code and logic ---------------------------------------------------------------------------------------


/**
 * Function that allows philosopher to think for THINKING_TIME seconds
 * @param Phil_ID_NUM: a philosopher that will be thinking
 */
void think(int Phil_ID_NUM, int TYPE) {
    // set state of philosopher to thinking
    state[Phil_ID_NUM] = THINKING;

    // determine message to print depending on the philosopher type
    if(TYPE == SYMMETRIC_PHILOSOPHER) {
        printf("Symmetric Philosopher #%d is Thinking for %d second\n\n", Phil_ID_NUM, THINKING_TIME);
    }else if (TYPE == ASYMMETRIC_PHILOSOPHER){
        printf("Asymmetric Philosopher #%d is Thinking for %d second\n\n", Phil_ID_NUM, THINKING_TIME);
    }
    // let the philosopher think for THINKING_TIME seconds
    sleep(THINKING_TIME);
}



/**
 * Function that allows a philosopher to eat for a "certain amount of time, which is drawn from a random quantum variable
 * with the average of avg eat time, to account for different eating times"
 */
void eat(int philosopher, int TYPE) {
    // set state of philosopher to eating
    state[philosopher] = EATING;

    // take the neighboring chopsticks
    chopsticks[philosopher] = TAKEN;

    if(TYPE == SYMMETRIC_PHILOSOPHER){
        printf("Symmetric philosopher # %d has picked up the left chopstick!\n", philosopher);
    }else if (TYPE == ASYMMETRIC_PHILOSOPHER) {
        printf("Asymmetric philosopher # %d has picked up the left chopstick!\n", philosopher);
    }

    chopsticks[(philosopher + NUM_ASTRONOMERS - 1) % NUM_ASTRONOMERS] = TAKEN;

    if(TYPE == SYMMETRIC_PHILOSOPHER){
        printf("Symmetric philosopher # %d has picked up the right chopstick!\n", philosopher);
    }else if (TYPE == ASYMMETRIC_PHILOSOPHER){
        printf("Asymmetric philosopher # %d has picked up the right chopstick!\n", philosopher);
    }

    // print chopstick status
    displayChopstick();

    // now unlock the mutex so other philosophers can have access to the monitor
    // If processes are waiting to enter the monitor then let them in first before unlocking the monitor for everyone else
    if (next_count > 0){
        // Signal the next process to come into the monitor, this allows the philosophers woken up by signal to now go
        sem_post(&next);
    }
    // otherwise just unlock the mutex for the monitor
    else{
        sem_post(&mutex);
        sem_post(&mutex2);
    }

    // determine how long to eat as described in the function description
    double eatTime = ((rand()/(float)RAND_MAX)*2*AVG_EAT_TIME);

    // determine message to print depending on the philosopher type
    if(TYPE == SYMMETRIC_PHILOSOPHER) {
        printf("Symmetric Philosopher #%d is eating for %.2f second\n\n", philosopher, eatTime);
    }else if (TYPE == ASYMMETRIC_PHILOSOPHER){
        printf("Asymmetric Philosopher #%d is eating for %.2f second\n\n", philosopher, eatTime);
    }
    // let the philosopher eat for eatTime seconds
    sleep((int)eatTime);

    // says the philosopher is done eating
    if(TYPE == SYMMETRIC_PHILOSOPHER) {
        printf("Symmetric Philosopher #%d is finished eating.\n\n", philosopher);
    }else if (TYPE == ASYMMETRIC_PHILOSOPHER){
        printf("Asymmetric Philosopher #%d is finished eating\n\n", philosopher);
    }
}



/**
 * Test if the philosopher to the left of philosopher with id number PHIL_ID_NUM is not eating and thus has a free chopstick.
 * The philosopher itself must also be thinking
 * @param PHIL_ID_NUM: A philosopher who's left neighbor we are interested in testing
 * @return: 1 if left neighbor is not eating, otherwise return 0.
 */
int testLeft(int PHIL_ID_NUM){
    if((state[(PHIL_ID_NUM + 1) % NUM_ASTRONOMERS] != EATING) && (state[PHIL_ID_NUM] == THINKING)){
        return 1;

    } else{
        return 0;
    }
}


/**
 * Test if the philosopher to the right of philosopher with id number PHIL_ID_NUM is not eating and thus has a free chopstick
 * @param PHIL_ID_NUM: A philosopher who's right neighbor we are interested in testing
 * @return: 1 if right neighbor is not eating, otherwise return 0.
 */
int testRight(int PHIL_ID_NUM){
    if((state[(PHIL_ID_NUM + NUM_ASTRONOMERS - 1) % NUM_ASTRONOMERS] != EATING) && (state[PHIL_ID_NUM] == THINKING)){
        return 1;

    } else{
        return 0;
    }

}



/**
 * Function to test both left and right neighbor of philosopher number: PHIL_ID_NUM, this will allow PHIL_ID_NUM to be
 * signalled and resumed from its waiting state if it's neighbors meets the requirements
 * @param PHIL_ID_NUM: Philosopher who's neighbors we are testing
 */
void testBoth(int PHIL_ID_NUM){
    if ((state[(PHIL_ID_NUM + NUM_ASTRONOMERS - 1) % NUM_ASTRONOMERS] != EATING) && (state[PHIL_ID_NUM] == THINKING) && (state[(PHIL_ID_NUM + 1) % NUM_ASTRONOMERS] != EATING)){
        // Does not do anything during pickup() because wait status is not waiting when you're picking stuff up. But when you're putting down a chopstick you
        // will wake up the neighbors
        signal(PHIL_ID_NUM);
    }
}




// function that prints out the status of each chopstick
void displayChopstick(){
    // create a variable to hold the display chopstick string
    // we will allocate 65 bytes per philosopher so we will never run out of room to store data
    char output [65 * NUM_ASTRONOMERS];

    // clearing display chopstick memory
    memset(output, 0, (65 * NUM_ASTRONOMERS));

    strncat(output, "Chopsticks: ", 13);

    for(int i = 0; i < NUM_ASTRONOMERS; i++){
        // print on next line if past 10 chopsticks already displayed
        if(((i % 10) == 0) && (i != 0)){
            strncat(output,"\n",sizeof ("\n"));
        }

        if(chopsticks[i] == FREE) {
            char temp [50 * NUM_ASTRONOMERS];
            sprintf(temp,"%d = Free, ", i);
            strncat(output, temp, (50*NUM_ASTRONOMERS));
        } else{
            char temp [50 * NUM_ASTRONOMERS];
            sprintf(temp,"%d = Taken, ", i);
            strncat(output, temp, (50*NUM_ASTRONOMERS));
        }
    }
    printf("%s\n\n",output);
}







// template for the philosopher thread
// it is ran by each thread
// simulates the behavior of the different type of philosophers. Takes an int array as an argument where
// array = {Philosopher#, Philosopher type} and philosopher type is asymmetric or symmetric.
void* philosopher(void* pointer) {
    // cast void pointer to int array pointer
    int* philosopherID = (int*) pointer;

    // get this philosophers number
    int Phil_ID_NUM = philosopherID[PHIL_NUM];
    // get this philosophers type
    int PHIL_TYP = philosopherID[PHIL_TYPE];

    // Loop forever
    while(1) {
        // start off thinking
        think(Phil_ID_NUM, PHIL_TYP);
        // try and pick up chop stick to eat
        pickup(Phil_ID_NUM, PHIL_TYP);
        // after done eating put down chopsticks
        putdown(Phil_ID_NUM, PHIL_TYP);
    }
    // never reached
    return NULL;
}



// a function to generate random ordering of 0 and 1
// 0 means symmetric, 1 means asymmetric
void place_astronomers(int *philOrdering){
    // generate random ordering
    // 0 means symmetric, 1 means asymmetric
    for (int i = 0; i < NUM_ASYMMETRIC; i++) {
        ordering[i] = 1;
    }
    for (int i = NUM_ASYMMETRIC; i < NUM_ASTRONOMERS; i++) {
        ordering[i] = 0;
    }
    // shuffle the ordering
    for (int i = 0; i < NUM_ASTRONOMERS; i++) {
        int j = rand() % NUM_ASTRONOMERS;
        int temp = ordering[i];
        ordering[i] = ordering[j];
        ordering[j] = temp;
    }
}



/**
 * Main function to simulate cosmic dining philosophers without bonus.
 * @return: 0 Upon success, anything else for error
 */
int main(){
    // seed random
    // time seed for srand
    time_t seedTime;
    // seed srand with current system time
    srand((unsigned)time(&seedTime));


    // initialize the semaphores in the monitor and philosopher states
    // use pshare 0 so it is shared between the threads, these semaphores must be global variables!
    sem_init(&mutex, 0, 1);  // mutex initialized to 1
    sem_init(&mutex2, 0, 1); //mutex 2 initialized to 1
    sem_init(&next, 0, 0);   // next initialized to 0 so it can allow a philosopher to wait right away

    // loop through every philosopher and set their states
    for (int i = 0; i < NUM_ASTRONOMERS; i++){
        // start each philosopher off thinking
        state[i] = THINKING;
        // initialize the behavior structure for each philosopher
        // their behavior semaphore is already preset to 0 so it can wait right away when called
        if(sem_init(&behavior[i].wait, 0, 0) == -1){
            perror("Error could not create semaphores required for critical section locking. Program exiting...\n\n");
            exit(1);
        }
        // set wait status to not waiting
        behavior[i].waitStatus = NOT_WAITING;

        // set each chopstick to be free
        chopsticks[i] = FREE;
    }

    // 2D array to correlate philosopher number to philosopher ordering, phil_id = {{Philosopher#, Philosopher type},...}
    int phil_id[NUM_ASTRONOMERS][2];

    // holds all of the philosopher threads
    pthread_t philosophers[NUM_ASTRONOMERS];

    // order the philosophers
    place_astronomers(ordering);

    // print program start message
    printf("Starting Dining Philosophers Program (p1.c non-bonus version). To stop the program press ctrl + c\n\n");

    // add a new line to terminal output
    printf("\n");
    // print chopstick status
    displayChopstick();

    // initialize threads, philosophers, and start them
    for (int i = 0; i < NUM_ASTRONOMERS; i++){
        // give each philosopher a number
        phil_id[i][PHIL_NUM] = i;
        // give that philosopher an ordering which defines if they are asymmetric or symmetric
        phil_id[i][PHIL_TYPE] = ordering[i];

        // Create Philosopher Processes, so we initialize a new philosopher thread to the philosopher routine, and also give its id array as well
        pthread_create(&philosophers[i], NULL, philosopher, &phil_id[i]);
    }


    // call join on philosophers
    for (int i = 0; i < NUM_ASTRONOMERS; i++){
        pthread_join(philosophers[i], NULL);
    }

    return 0;
}