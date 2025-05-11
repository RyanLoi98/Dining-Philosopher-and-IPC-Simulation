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


/*
 * CPSC 457 Assignment 2
 * Section 1: Part 3
 *
 * Ryan Loi
 * Lecture: 02
 * Tutorial: T10
 */

// memory size is 1 page
#define MEM_SIZE 4096
// turns for parent and child (for the flag system)
#define PARENT_TURN 0
#define CHILD_TURN 1
#define STORE_SIZE 1000

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



/**
 * Function that converts a string to all uppercase letters
 * @param parentMSGUP: string to be converted to uppercase
 */
void uppercase (char parentMSGUP[]){
    // loop through message and convert to uppercase character by character
    for(int i = 0; parentMSGUP[i] != '\0'; i++ ){
        parentMSGUP[i] = toupper(parentMSGUP[i]);
    }
}


// Data structure for the shared memory
struct sharedData {
    // who is going next: 0 = parent, 1 = child
    int flag;
    // stores messages between the child and parent
    char message[STORE_SIZE];
};

/**
 * Function that simulates parent child process IPC using shared memory and semaphores for synchronization
 * @return 0 upon success, 1 upon failure
 */
int main(){
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

    // let parent go first
    block->flag = PARENT_TURN;


    // release block
    if ((shmdt(block)) == -1){
        perror("Error unable to release memory block. Program exiting...\n\n");
        exit(1);
    }


    // Fork to create a child process
    int child_pid = fork();

    // check if fork had an error, exit (return) with error code 1 if this is the case
    if(child_pid == -1){
        perror("Error: Unable to fork... Program shutting down...\n\n");
        return 1;
    }

    // child
    if(child_pid == 0){
        // get prepare semaphoreBuffer for locking
        locking(&semaphoreBuffer);
        // attempt to obtain lock
        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to obtain semaphore lock. Program exiting...\n\n");
            // kill parent
            kill(getppid(), SIGKILL);
            exit(1);
        }

        // get message from parent, first acquire the block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // kill parent
            kill(getppid(), SIGKILL);
            exit(1);
        }

        // wait until child turn
        while(block->flag != CHILD_TURN);


        // copy the message over from the parent
        char recievedMSG [STORE_SIZE];
        memcpy(recievedMSG, block->message, STORE_SIZE);

        // print message
        printf("Child Process: Recieved the following message from parent - \"%s\"\n", block->message);

        // convert message to uppercase
        uppercase(recievedMSG);

        // print parent message to all uppercase
        printf("Child Process: Converted parent's message to all upper case - %s\n", recievedMSG);

        // child's message
        char childMSG[STORE_SIZE];

        // getting child's pid then convert it to a string before concatenating it to the response message
        int c_pid = getpid();
        sprintf(childMSG, "OK sure, my pid is: ");
        char convertedPID[10];
        sprintf(convertedPID, "%d", c_pid);
        strcat(childMSG, convertedPID);

        // print message that will be sent to parent
        printf("Child Process: Responding to parent with message - \"%s\"\n", childMSG);

        // copy a message to the block
        memcpy(block->message, childMSG, STORE_SIZE);

        printf("Child successfully sent response to the parent via the shared memory.\n\n");

        block->flag = PARENT_TURN;


        // prepare semaphoreBuffer for unlocking
        unlocking(&semaphoreBuffer);

        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to unlock semaphore. Program exiting...\n\n");
            // kill parent
            kill(getppid(), SIGKILL);
            exit(1);
        }


        // get prepare semaphoreBuffer for locking
        locking(&semaphoreBuffer);
        // attempt to obtain lock
        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to obtain semaphore lock. Program exiting...\n\n");
            // kill parent
            kill(getppid(), SIGKILL);
            exit(1);
        }

        // wait till child turn
        while (block->flag != CHILD_TURN);

        // copy the message over from the parent
        memcpy(recievedMSG, block->message, STORE_SIZE);

        // print message
        printf("Child Process: Recieved the following message from parent - \"%s\"\n", block->message);

        // convert message to uppercase
        uppercase(recievedMSG);

        // print parent message to all uppercase
        printf("Child Process: Converted parent's message to all upper case - %s\n", recievedMSG);


        char parentTest[25] = "You are indeed my child!";
        if(strcmp(parentTest, block->message) == 0){
            sprintf(childMSG, "%s", "Hi Parent! I love you!");

        } else{
            sprintf(childMSG, "%s", "Oh no where are my parents!");
        }
        // print message that will be sent to parent
        printf("Child process: Responding to the parent with message - \"%s\"\n", childMSG);

        // copy a message to the block
        memcpy(block->message, childMSG, STORE_SIZE);

        printf("Child successfully sent response to the parent via the shared memory.\n\n");

        // make it parents turn
        block->flag = PARENT_TURN;

        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill parent
            kill(getppid(), SIGKILL);
            exit(1);
        }

        // prepare semaphoreBuffer for unlocking
        unlocking(&semaphoreBuffer);

        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to unlock semaphore. Program exiting...\n\n");
            // kill parent
            kill(getppid(), SIGKILL);
            exit(1);
        }






        // parent
    } else{
        // parent can go first without having to acquire a lock because the semaphore was prelocked for it

        // get block
        if(( block = (sharedData*)shmat(memory_id, NULL, 0)) == (void*) -1){
            perror("Error unable to acquire the memory pointer. Program exiting...\n\n");
            // kill child
            kill(child_pid, SIGKILL);
            exit(1);
        }

        // wait for parent turn
        while (block->flag != PARENT_TURN);

        // message for the block
        char parentMSG [STORE_SIZE] = {"Hello there, can you give me your P_ID so I can check if you're my child?"};

        // copy a message to the block
        memcpy(block->message, parentMSG, STORE_SIZE);

        // print message to be sent to the child process
        printf("Parent Process: Sending the following message to the child process - \"%s\"\n", block->message);


        printf("Parent successfully sent message to the child via the shared memory.\n\n");

        // make it childs turn
        block->flag = CHILD_TURN;

        // prepare semaphoreBuffer for unlocking
        unlocking(&semaphoreBuffer);
        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to unlock semaphore. Program exiting...\n\n");
            // kill parent
            kill(child_pid, SIGKILL);
            exit(1);
        }


        // get prepare semaphoreBuffer for locking
        locking(&semaphoreBuffer);
        // attempt to obtain lock
        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to obtain semaphore lock. Program exiting...\n\n");
            // kill child
            kill(child_pid, SIGKILL);
            exit(1);
        }


        // wait till parent turn
        while (block->flag != PARENT_TURN);

        // copy the message over from the child
        char recievedMSG [STORE_SIZE];
        memcpy(recievedMSG, block->message, STORE_SIZE);

        // print message
        printf("Parent Process: Recieved the following message from child - \"%s\"\n", block->message);

        // Parent confirms if the child is their child via a pid check
        char childTest [50] = {"OK sure, my pid is: "};
        // convert child pid to string
        char childPidStr [12];
        sprintf(childPidStr, "%d", child_pid);
        strcat(childTest, childPidStr);

        // test if the child belongs to the parent by checking its pid
        if(strcmp(childTest, recievedMSG) == 0){
            printf("Parent Process: The parent process has confirmed that the child process belongs to it\n");
            // set response message
            sprintf(parentMSG, "%s" ,"You are indeed my child!");

        } else{
            printf("Parent Process: The parent process has confirmed that the child process doesn't belongs to it\n");
            // set response message
            sprintf(parentMSG, "%s" ,"You are not my child!");
        }


        // print message to be sent to the child process
        printf("Parent Process: Sending the following message to the child process - \"%s\"\n", parentMSG);

        // copy a message to the block
        memcpy(block->message, parentMSG, STORE_SIZE);

        printf("Parent successfully sent message to the child via the shared memory.\n\n");

        // make it child's turn
        block->flag = CHILD_TURN;



        // prepare semaphoreBuffer for unlocking
        unlocking(&semaphoreBuffer);
        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to unlock semaphore. Program exiting...\n\n");
            // kill parent
            kill(child_pid, SIGKILL);
            exit(1);
        }



        // get prepare semaphoreBuffer for locking
        locking(&semaphoreBuffer);
        // attempt to obtain lock
        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to obtain semaphore lock. Program exiting...\n\n");
            // kill parent
            kill(child_pid, SIGKILL);
            exit(1);
        }


        // wait until parent turn
        while (block->flag != PARENT_TURN);

        memcpy(recievedMSG, block->message, STORE_SIZE);

        // print message
        printf("Parent Process: Recieved the following message from child - \"%s\"\n", block->message);
        printf("Program finished...\n");


        // release block
        if ((shmdt(block)) == -1){
            perror("Error unable to release memory block. Program exiting...\n\n");
            // kill child
            kill(child_pid, SIGKILL);
            exit(1);
        }


        // prepare semaphoreBuffer for unlocking
        unlocking(&semaphoreBuffer);
        if (semop(sem_id, &semaphoreBuffer, 1) == -1){
            perror("Error unable to unlock semaphore. Program exiting...\n\n");
            // kill parent
            kill(child_pid, SIGKILL);
            exit(1);
        }


    }


    // wait for the child
    wait(NULL);

    // allow only the parent to remove the semaphore and allocated memory
    if(child_pid != 0) {
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