#include <stdio.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

/*
 * CPSC 457 Assignment 2
 * Section 1: Part 4
 *
 * Ryan Loi
 * Lecture: 02
 * Tutorial: T10
 */


// DEFINITIONS
// memory size is 1 page
#define MEM_SIZE 4096
// max storage size
#define STORE_SIZE 1000
// Child's status
#define MSG_PENDING 0
#define MSG_SENT 1
// Child's index for array access
#define child_1_index 0
#define child_2_index 1
#define child_3_index 2

//Global PID's
int parentPID;
int child_1_PID;
int child_2_PID;
int child_3_PID;




// predeclaring the shared data struct
typedef struct sharedData sharedData;

/**
 * Function that generates the locking semaphore buffer
 * @param semaphoreBuffer: semaphore buffer
 */
void locking(struct sembuf* semaphoreBuffer){
    semaphoreBuffer->sem_op = -1;
}



/**
 * Function that generates the unlocking semaphore buffer
 * @param semaphoreBuffer: semaphore buffer
 */
void unlocking(struct sembuf* semaphoreBuffer){
    semaphoreBuffer->sem_op = 1;
}


// Data structure for the shared memory
struct sharedData {
    //array for child's message status, 3 spots for the 3 children
    int childMSGStatus[3];

    // stores messages between the child and parent
    char messages[3][STORE_SIZE];

    // termination signal to signal graceful termination
    int terminate;

    // an array to hold all the pid's in the program so it can be transferred to each child.
    int pids [3];

    // variable to signal if the pid array has been set by parent yet
    int pidSet;

    // flags to show that the processes have set their pids. Only child 1 and 2 need to do this as they will not automatically
    // inherit all of the other pid's
    int child1flag;
    int child2flag;
};

/**
 * Function that simulates parent and 3 child process IPC using shared memory with process coordination and synchronization
 * @return 0 upon success, 1 upon failure
 */
int main() {
    // get parentPID
    parentPID = getpid();

    // key for semaphore
    key_t sem_key;
    // semaphore id
    int sem_id;

    // buffer for the semaphore (sem num = 0, sem_op = 0, sem_flg = 0
    struct sembuf semaphoreBuffer = {0, -1, 0};

    // generate key for semaphore
    if ((sem_key = ftok("./", 1)) == -1){
        perror("Error could not generate semaphore key. Program exiting...\n\n");
        exit(-1);
    }

    // get semaphore id
    if ((sem_id = semget(sem_key, 1, 0666 | IPC_CREAT)) == -1){
        perror("Could not get semaphore id. Program Exiting...\n\n");
        exit(-1);
    }

    // Initialize semaphore to the value of 1
    if ((semctl(sem_id, 0, SETVAL, 1)) == -1){
        perror("Cannot initialize the semaphore. Exiting the program...\n\n");
        exit(1);
    }


    // key for shared memory
    key_t mem_key;
    // shared memory id
    int memory_id;
    // pointer block for memory access
    sharedData *block;

    // generate key for shared memory
    if ((mem_key = ftok("./", 2)) == -1){
        perror("Error could not generate memory key. Program exiting...\n\n");
        exit(-1);
    }

    // allocate memory segment and return its identifier
    if((memory_id = shmget(mem_key, MEM_SIZE, 0644 | IPC_CREAT)) == -1){
        perror("Error could not allocate memory. Program exiting...\n\n");
        exit(-1);
    }


    // put data structure into the shared memory
    // get pointer
    if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
        perror("Error unable to acquire the memory pointer. Program exiting...\n\n");

        exit(1);
    }

    // setup the block

    // first setup the array indicating the child's status
    block->childMSGStatus[child_1_index] = MSG_PENDING;
    block->childMSGStatus[child_2_index] = MSG_PENDING;
    block->childMSGStatus[child_3_index] = MSG_PENDING;

    // signal that the memory block's pid array has not yet been set
    block->pidSet = 0;

    // set the child flags to 0 as none of them have inherited all the pids yet
    block->child1flag = 0;
    block->child2flag = 0;


    // set the terminate flag to false (0)
    block->terminate = 0;

    // release block
    if ((shmdt(block)) == -1){
        perror("Error unable to release memory block. Program exiting...\n\n");
        // kill parent
        kill(getppid(), SIGKILL);
        exit(1);
    }


    // CHILD1
    // Fork to create a child process
    child_1_PID = fork();

    // check if fork had an error, exit (return) with error code 1 if this is the case
    if(child_1_PID == -1){
        perror("Error: Unable to fork... Program shutting down...\n\n");
        return 1;
    }

    // inside of the first child assign its own pid
    if(getpid()!=parentPID){
        child_1_PID = getpid();
    }


    //CHILD 2
    // inside of the parent create the second child, this ensures it is a child of the parent only and not a grandchild
    if(getpid() == parentPID){
        child_2_PID = fork();

        // check if fork had an error, exit (return) with error code 1 if this is the case
        if(child_2_PID == -1){
            perror("Error: Unable to fork... Program shutting down...\n\n");
            return 1;
        }
    }

    // inside of the second child assign its pid
    if((getpid() != parentPID) && (getpid() != child_1_PID)){
        child_2_PID = getpid();
    }



    // CHILD3
    // inside of the parent only create the third child, this ensures it is a child of the parent only and not a grandchild
    if(getpid() == parentPID){
        child_3_PID = fork();

        // check if fork had an error, exit (return) with error code 1 if this is the case
        if(child_3_PID == -1){
            perror("Error: Unable to fork... Program shutting down...\n\n");
            return 1;
        }
    }

    //inside of the third child assign its pid
    if((getpid() != parentPID) && (getpid() != child_1_PID) && (getpid() != child_2_PID)){
        child_3_PID = getpid();
    }


    // make sure all children have all other pids

    // first the parent will have to assign the pid's to the shared memory
    if(getpid() == parentPID){
        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // kill all children
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }

        // assign pids to the memory block
        block->pids[child_1_index] = child_1_PID;
        block->pids[child_2_index] = child_2_PID;
        block->pids[child_3_index] = child_3_PID;


        // trip the set flag as the pids have been set
        block->pidSet = 1;

        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill all children
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }


     // for child 1
    } else if(getpid() == child_1_PID){
        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // exit
            exit(1);
        }

        // Creating Timer so the child can check if the parent is finished with the pid block yet
        // Variable to hold initial time
        time_t time1;
        time(&time1);
        // variable to hold current time
        time_t time2;
        // variable to hold amount of elapsed time
        double diffTime = 0;

        // status on if both children got pid from memory 1 = true, 0 = false
        int status = 0;

        // loop for 2 seconds to keep checking if parent has finished with the pids yet
        do{
            // get current time
            time(&time2);
            // calculate elapsed time
            diffTime = difftime(time2, time1);

            if((block->pidSet) == 1){
                status = 1;
                break;
            }

        } while (diffTime <= 2);

        // if the children do not have pids from the memory then kill the program
        if(status == 0){
            perror("Error: Children were unable to get their PID's from the shared memory. Program exiting...\n\n");
            exit(1);
        }

        // assign pid from memory block
        child_2_PID = block->pids[child_2_index];
        child_3_PID = block->pids[child_3_index];


        // trip the set flag for child 1
        block->child1flag = 1;

        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill all children
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }


        // for child 2
    } else if(getpid() == child_2_PID){

        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // exit
            exit(1);
        }

        // Creating Timer so the child can check if the parent is finished with the pid block yet
        // Variable to hold initial time
        time_t time1;
        time(&time1);
        // variable to hold current time
        time_t time2;
        // variable to hold amount of elapsed time
        double diffTime = 0;

        // status on if both children got pid from memory 1 = true, 0 = false
        int status = 0;

        // loop for 2 seconds to keep checking if parent has finished with the pids yet
        do{
            // get current time
            time(&time2);
            // calculate elapsed time
            diffTime = difftime(time2, time1);

            if((block->pidSet) == 1){
                status = 1;
                break;
            }

        } while (diffTime <= 2);

        // if the children do not have pids from the memory then kill the program
        if(status == 0){
            perror("Error: Children were unable to get their PID's from the shared memory. Program exiting...\n\n");
            exit(1);
        }

        // assign pid from memory block
        child_3_PID = block->pids[child_3_index];


        // trip the set flag for child 1
        block->child2flag = 1;

        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill all children
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }
    }

    // now for the parent to make sure all the children got the pids
    if(getpid() == parentPID){
        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // exit
            exit(1);
        }

        // Creating Timer so the program so we can monitor if the parent has set the flag to let the children go and read the pids yet
        // Variable to hold initial time
        time_t time1;
        time(&time1);
        // variable to hold current time
        time_t time2;
        // variable to hold amount of elapsed time
        double diffTime = 0;

        // status on if both children got pid from memory 1 = true, 0 = false
        int status = 0;

        // loop for 2 seconds to keep checking if children have gotten the pids
        do{
            // get current time
            time(&time2);
            // calculate elapsed time
            diffTime = difftime(time2, time1);

            if((block->child1flag == 1) && (block->child2flag == 1)){
                status = 1;
                break;
            }

        } while (diffTime <= 2);

        // if the children do not have pids from the memory then kill the program
        if(status == 0){
            perror("Error: Children were unable to get their PID's from the shared memory. Program exiting...\n\n");
            // kill all children
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }

        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill all children
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }
    }



// begin the program


    // for the parent
    if(getpid() == parentPID){

        printf("Welcome to the demonstration program for part 4 - IPC using shared memory with process coordination and synchronization.\n");
        printf("This program will run for 15 seconds. While doing so the following output from the children will be displayed:\n"
               "child 1 will be counting by 1's, child 2 will be counting by 2's and child 3 will be counting by 3's\n\n");


        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // kill all children
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }

        // wait for a second so all the children can catch up
        sleep(1);

        // Creating Timer so the program so we can allow the program to only run for 10 seconds.
        // Variable to hold initial time
        time_t initialTime;
        time(&initialTime);
        // variable to hold current time
        time_t currentTime;
        // variable to hold amount of elapsed time
        double elapsedTime = 0;



        // loop for 15 seconds
        do{
            // get current time
            time(&currentTime);
            // calculate elapsed time
            elapsedTime = difftime(currentTime, initialTime);

            // parent scans the array for the 1st child, if the child has a sent message enter if statement
            if ((block->childMSGStatus[child_1_index]) == MSG_SENT){
                // print child message
                printf("Parent Process: Child # 1 sent: %s\n\n",block->messages[child_1_index]);
                // update child status
                block->childMSGStatus[child_1_index] = MSG_PENDING;
            }

            // parent scans the array for the 2nd child, if the child has a sent message enter if statement
            if ((block->childMSGStatus[child_2_index]) == MSG_SENT){
                // print child message
                printf("Parent Process: Child # 2 sent: %s\n\n",block->messages[child_2_index]);
                // update child status
                block->childMSGStatus[child_2_index] = MSG_PENDING;
            }

            // parent scans the array for the 3rd child, if the child has a sent message enter if statement
            if ((block->childMSGStatus[child_3_index]) == MSG_SENT){
                // print child message
                printf("Parent Process: Child # 3 sent: %s\n\n",block->messages[child_3_index]);
                // update child status
                block->childMSGStatus[child_3_index] = MSG_PENDING;
            }

            // added a space to output
            printf("\n");

            // sleep a bit so the parent only periodically checks the children
            sleep(2);

        } while (elapsedTime <= 15);

        // if time elapsed then we must set the flag
        block->terminate = 1;

        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill all children
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }



     // for child 1
    }else if(getpid() == child_1_PID){

        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // kill all processes
            kill(parentPID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }

        // counter for child to count
        int counter = 0;

        // loop forever
        while (1){
            // break out of the loop when the parent sets the terminate signal
            if(block->terminate){
                // prepare semaphoreBuffer for unlocking
                unlocking(&semaphoreBuffer);
                if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                    perror("Error unable to unlock semaphore. Program exiting...\n\n");
                    // kill all processes
                    kill(parentPID, SIGKILL);
                    kill(child_2_PID, SIGKILL);
                    kill(child_3_PID, SIGKILL);
                    exit(1);
                }
                break;
            }

            // get prepare semaphoreBuffer for locking
            locking(&semaphoreBuffer);
            // attempt to obtain lock
            if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                perror("Error unable to obtain semaphore lock. Program exiting...\n\n");
                // kill all processes
                kill(parentPID, SIGKILL);
                kill(child_2_PID, SIGKILL);
                kill(child_3_PID, SIGKILL);
                exit(1);
            }

            // set the message
            if((block->childMSGStatus[child_1_index]) == MSG_PENDING){
                // increment counter
                counter ++;

                // string counter
                char strCounter[STORE_SIZE];
                sprintf(strCounter, "%d", counter);

                strncpy(block->messages[child_1_index], strCounter, STORE_SIZE);

                // update the status of the child
                block->childMSGStatus[child_1_index] = MSG_SENT;
            }

            // prepare semaphoreBuffer for unlocking
            unlocking(&semaphoreBuffer);
            if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                perror("Error unable to unlock semaphore. Program exiting...\n\n");
                // kill all processes
                kill(parentPID, SIGKILL);
                kill(child_2_PID, SIGKILL);
                kill(child_3_PID, SIGKILL);
                exit(1);
            }
        }


        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill all processes
            kill(parentPID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }


     // for child 2
    }else if (getpid() == child_2_PID){

        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // kill all processes
            kill(parentPID, SIGKILL);
            kill(child_1_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }

        // counter for child to count
        int counter = 0;

        // loop forever
        while (1){
            // break out of the loop when the parent sets the terminate signal
            if(block->terminate){
                // prepare semaphoreBuffer for unlocking
                unlocking(&semaphoreBuffer);
                if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                    perror("Error unable to unlock semaphore. Program exiting...\n\n");
                    // kill all processes
                    kill(parentPID, SIGKILL);
                    kill(child_1_PID, SIGKILL);
                    kill(child_3_PID, SIGKILL);
                    exit(1);
                }
                break;
            }

            // get prepare semaphoreBuffer for locking
            locking(&semaphoreBuffer);
            // attempt to obtain lock
            if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                perror("Error unable to obtain semaphore lock. Program exiting...\n\n");
                // kill all processes
                kill(parentPID, SIGKILL);
                kill(child_1_PID, SIGKILL);
                kill(child_3_PID, SIGKILL);
                exit(1);
            }

            // set the message
            if((block->childMSGStatus[child_2_index]) == MSG_PENDING){
                // increment counter
                counter += 2;

                // string counter
                char strCounter[STORE_SIZE];
                sprintf(strCounter, "%d", counter);

                strncpy(block->messages[child_2_index], strCounter, STORE_SIZE);

                // update the status of the child
                block->childMSGStatus[child_2_index] = MSG_SENT;
            }

            // prepare semaphoreBuffer for unlocking
            unlocking(&semaphoreBuffer);
            if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                perror("Error unable to unlock semaphore. Program exiting...\n\n");
                // kill all processes
                kill(parentPID, SIGKILL);
                kill(child_1_PID, SIGKILL);
                kill(child_3_PID, SIGKILL);
                exit(1);
            }


        }


        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill all processes
            kill(parentPID, SIGKILL);
            kill(child_1_PID, SIGKILL);
            kill(child_3_PID, SIGKILL);
            exit(1);
        }



    // for child 3
    }else if(getpid() == child_3_PID){

        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // kill all processes
            kill(parentPID, SIGKILL);
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            exit(1);
        }

        // counter for child to count
        int counter = 0;

        // loop forever
        while (1){
            // break out of the loop when the parent sets the terminate signal
            if(block->terminate){
                // prepare semaphoreBuffer for unlocking
                unlocking(&semaphoreBuffer);
                if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                    perror("Error unable to unlock semaphore. Program exiting...\n\n");
                    // kill all processes
                    kill(parentPID, SIGKILL);
                    kill(child_1_PID, SIGKILL);
                    kill(child_2_PID, SIGKILL);
                    exit(1);
                }
                break;
            }

            // get prepare semaphoreBuffer for locking
            locking(&semaphoreBuffer);
            // attempt to obtain lock
            if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                perror("Error unable to obtain semaphore lock. Program exiting...\n\n");
                // kill all processes
                kill(parentPID, SIGKILL);
                kill(child_1_PID, SIGKILL);
                kill(child_2_PID, SIGKILL);
                exit(1);
            }

            // set the message
            if((block->childMSGStatus[child_3_index]) == MSG_PENDING){
                // increment counter
                counter += 3;

                // string counter
                char strCounter[STORE_SIZE];
                sprintf(strCounter, "%d", counter);

                strncpy(block->messages[child_3_index], strCounter, STORE_SIZE);

                // update the status of the child
                block->childMSGStatus[child_3_index] = MSG_SENT;
            }


            // prepare semaphoreBuffer for unlocking
            unlocking(&semaphoreBuffer);
            if (semop(sem_id, &semaphoreBuffer, 1) == -1){
                perror("Error unable to unlock semaphore. Program exiting...\n\n");
                // kill all processes
                kill(parentPID, SIGKILL);
                kill(child_1_PID, SIGKILL);
                kill(child_2_PID, SIGKILL);
                exit(1);
            }


        }


        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill all processes
            kill(parentPID, SIGKILL);
            kill(child_1_PID, SIGKILL);
            kill(child_2_PID, SIGKILL);
            exit(1);
        }
    }


    // waiting for all children processes to return
    while(wait(NULL) != -1 || errno != ECHILD){
    }

    // allow only the parent to remove the semaphore and allocated memory
    if(getpid() == parentPID) {
        // Remove semaphore
        if (semctl(sem_id, 0, IPC_RMID) == -1) {
            perror("Error could not delete the semaphore. Program Exiting...\n\n");
            exit(1);
        }
        // remove allocated memory
        if((shmctl(memory_id, IPC_RMID, NULL)) == -1){
            perror("Error could not remove the shared memory block. Program Exiting...\n\n");
            exit(1);
        }
    }

    return 0;
}
