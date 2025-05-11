#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
/*
 * CPSC 457 Assignment 2
 * Section 1: Part 2
 *
 * Ryan Loi
 * Lecture: 02
 * Tutorial: T10
 */

// Max Message Size
#define MAX_MESSAGE_SIZE 512

/**
 * Structure for the message queue
 */
struct message_queue {
    long int message_type;
    char message_text[MAX_MESSAGE_SIZE];
};


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



/**
 * Function that simulates parent child process IPC using message queues
 * @return 0 upon success, 1 upon failure
 */
int main() {
    // Fork to create a child process
    int child_pid = fork();

    // check if fork had an error, exit (return) with error code 1 if this is the case
    if(child_pid == -1){
        perror("Error: Unable to fork... Program shutting down...\n\n");
        return 1;
    }

    // create message queue for both the parent and the child

    // message type for parent to child
    long int pc = 1;
    // message type for child to parent
    long int cp= 2;

    // initialize 2 message buffers, one for parent to child and one for child to parent communication
    struct message_queue parent2child;
    struct message_queue child2parent;

    // set their specific message type
    parent2child.message_type = pc;
    child2parent.message_type = cp;

    // clear buffers
    memset(parent2child.message_text, 0, sizeof (parent2child.message_text));
    memset(child2parent.message_text, 0, sizeof(child2parent.message_text));

    // message queue for sending message from the parent to the child
    // message queue id
    int message_id;

    // keys
    key_t key;
    key_t key2;

    // generate unique key for the message queue
    if((key = ftok("./", 1)) == -1){
        perror("Error: Key could not be created. Program Exiting...\n\n");
        // kill other processes
        if(child_pid == 0){
            kill(getppid(), SIGKILL);
        }else{
            kill(child_pid, SIGKILL);
        }
        exit(-1);
    }

    // generate unique key for the message queue
    if((key2 = ftok("./", 2)) == -1){
        perror("Error: Key 2 could not be created. Program Exiting...\n\n");
        // kill other processes
        if(child_pid == 0){
            kill(getppid(), SIGKILL);
        }else{
            kill(child_pid, SIGKILL);
        }

        exit(-1);
    }

    // Creating a message queue in memory with permission 0666. Returns a message queue id
    message_id = msgget(key, 0666 | IPC_CREAT);

    // Check if message id == -1 signifying an error occurred and the queue could not be created, so print error and exit
    // with error code -1
    if (message_id == -1){
        perror("Error: Message Queue could not be created. Program exiting...\n\n");
        // kill other processes
        if(child_pid == 0){
            kill(getppid(), SIGKILL);
        }else{
            kill(child_pid, SIGKILL);
        }

        exit(-1);
    }


    // message queue for sending message from the child to the parent
    // message queue id
    int message_id2;


    // Creating a message queue in memory with permission 0666. Returns a message queue id
    message_id2 = msgget(key2, 0666 | IPC_CREAT);

    // Check if message id == -1 signifying an error occurred and the queue could not be created, so print error and exit
    // with error code -1
    if (message_id2 == -1){
        perror("Error: Message Queue could not be created. Program exiting...\n\n");
        // kill other processes
        if(child_pid == 0){
            kill(getppid(), SIGKILL);
        }else{
            kill(child_pid, SIGKILL);
        }
        exit(1);
    }


    // for the child
    if(child_pid == 0){
        // string for child to store parent's message converted to uppercase
        char parentMSGUP[MAX_MESSAGE_SIZE];


        // string to store child's response
        char responseMSG[MAX_MESSAGE_SIZE];


        // receive message from message id queue (for parent to child only)
        // Reading from message queue
        if(msgrcv(message_id, (void *)&parent2child, MAX_MESSAGE_SIZE, pc, 0) == -1){
            perror("Error: Child unable to receive message from the parent through the message queue. Program exiting...\n\n");
            // destroy message queue
            msgctl(message_id, IPC_RMID, 0);
            msgctl(message_id2, IPC_RMID, 0);
            // kill parent process before exiting
            kill(getppid(), SIGKILL);

            exit(1);

        }else{
        printf("Child Process: Recieved the following message from parent - \"%s\"\n", parent2child.message_text);
        }


        // convert parent message to upper case
        strncpy(parentMSGUP, parent2child.message_text, MAX_MESSAGE_SIZE);

        // convert parent message to uppercase
        uppercase(parentMSGUP);

        // print parent message to all uppercase
        printf("Child Process: Converted parent's message to all upper case - %s\n", parentMSGUP);

        // getting child's pid then convert it to a string before concatenating it to the response message
        int c_pid = getpid();
        sprintf(responseMSG, "OK sure, my pid is: ");
        char convertedPID[10];
        sprintf(convertedPID, "%d", c_pid);
        strcat(responseMSG, convertedPID);

        // print message that will be sent to parent
        printf("Child Process: Responding to parent with message - \"%s\"\n", responseMSG);

        // copy message to the message structure
        strncpy(child2parent.message_text, responseMSG, MAX_MESSAGE_SIZE);


        // send response message to the parent using message id 2
        // send message to parent using message id2 queue (for child to parent only)
        // if send -1 print error message, destroy queue, and exit with error code 1
        if (msgsnd(message_id2, (void *)&child2parent, MAX_MESSAGE_SIZE, 0) == -1){
            perror("Error: Child unable to send message to the parent through the message queue. Program exiting...\n\n");
            // destroy message queue
            msgctl(message_id, IPC_RMID, 0);
            msgctl(message_id2, IPC_RMID, 0);
            // kill parent process before exiting
            kill(getppid(), SIGKILL);

            exit(1);
        }
            // otherwise print success message
        else{
            printf("Child successfully sent response to the parent through the message queue.\n\n");
        }


        // receive message from message id queue (for parent to child only)
        // Reading from message queue
        if(msgrcv(message_id, (void *)&parent2child, MAX_MESSAGE_SIZE, pc, 0) == -1){
            perror("Error: Child unable to receive message from the parent through the message queue. Program exiting...\n\n");
            // destroy message queue
            msgctl(message_id, IPC_RMID, 0);
            msgctl(message_id2, IPC_RMID, 0);
            // kill parent process before exiting
            kill(getppid(), SIGKILL);

            exit(1);
        }else {
            printf("Child Process: Recieved the following message from parent - \"%s\"\n", parent2child.message_text);
        }

        // convert parent message to upper case
        strncpy(parentMSGUP, parent2child.message_text, MAX_MESSAGE_SIZE);

        // convert parent message to uppercase
        uppercase(parentMSGUP);

        // print parent message to all uppercase
        printf("Child Process: Converted parent's message to all upper case - %s\n", parentMSGUP);


        // Respond to parent
        char parentTest[25] = "You are indeed my child!";
        if(strcmp(parentTest, parent2child.message_text) == 0){
            sprintf(responseMSG, "%s", "Hi Parent! I love you!");

        } else{
            sprintf(responseMSG, "%s", "Oh no where are my parents!");
        }

        printf("Child process: Responding to the parent with message - \"%s\"\n", responseMSG);

        // copy message to the message structure
        strncpy(child2parent.message_text, responseMSG, MAX_MESSAGE_SIZE);


        // send response message to the parent using message id 2
        // send message to parent using message id2 queue (for child to parent only)
        // if send -1 print error message, destroy queue, and exit with error code 1
        if (msgsnd(message_id2, (void *)&child2parent, MAX_MESSAGE_SIZE, 0) == -1){
            perror("Error: Child unable to send message to the parent through the message queue. Program exiting...\n\n");
            // destroy message queue
            msgctl(message_id2, IPC_RMID, 0);

            // kill parent process
            kill(getppid(), SIGKILL);

            exit(1);
        }
            // otherwise print success message
        else{
            printf("Child successfully sent response to the parent through the message queue.\n\n");
        }



        // for the parent
    } else{
        // message from the parent
        char parentMSG [MAX_MESSAGE_SIZE] = {"Hello there, can you give me your P_ID so I can check if you're my child?"};

        // print message to be sent to the child process
        printf("Parent Process: Sending the following message to the child process - \"%s\"\n", parentMSG);

        // copy message to the message structure
        strncpy(parent2child.message_text, parentMSG, MAX_MESSAGE_SIZE);

        // send message to child using message id queue (for parent to child only)
        // if send -1 print error message, destroy queue, and exit with error code 1
        if (msgsnd(message_id, (void *)&parent2child, MAX_MESSAGE_SIZE, 0) == -1){
            perror("Error: Parent unable to send message to the child through the message queue. Program exiting...\n\n");
            // destroy message queue
            msgctl(message_id, IPC_RMID, 0);
            msgctl(message_id2, IPC_RMID, 0);

            // kill child process
            kill(child_pid, SIGKILL);
            exit(1);
        }
        // otherwise print success message
        else{
            printf("Parent successfully sent message to the child through the message queue.\n\n");
        }


        // Receiving response from the child using message id queue 2 (for child to parent only)
        // Reading from message queue
        if(msgrcv(message_id2, (void *)&child2parent, MAX_MESSAGE_SIZE, cp, 0) == -1){
            perror("Error: Parent unable to receive message from the child through the message queue. Program exiting...\n\n");
            // destroy message queue
            msgctl(message_id, IPC_RMID, 0);
            msgctl(message_id2, IPC_RMID, 0);

            // kill child process
            kill(child_pid, SIGKILL);
            exit(1);
        }else{
        printf("Parent Process: Recieved the following response from the child - \"%s\"\n", child2parent.message_text);
        }

        // Parent confirms if the child is their child via a pid check
        char childTest [50] = {"OK sure, my pid is: "};
        // convert child pid to string
        char childPidStr [12];
        sprintf(childPidStr, "%d", child_pid);
        strcat(childTest, childPidStr);

        // test if the child belongs to the parent by checking its pid
        if(strcmp(childTest, child2parent.message_text) == 0){
            printf("Parent Process: The parent process has confirmed that the child process belongs to it\n");
            // set response message
            sprintf(parentMSG, "%s" ,"You are indeed my child!");

        } else{
            printf("Parent Process: The parent process has confirmed that the child process doesn't belongs to it\n");
            // set response message
            sprintf(parentMSG, "%s" ,"You are not my child!");
        }

        // display response message
        printf("Parent Process: Sending the following message to the child process - \"%s\"\n", parentMSG);

        // send response message to the child
        // copy message to the message structure
        strncpy(parent2child.message_text, parentMSG, MAX_MESSAGE_SIZE);


        // send message to child using message id queue (for parent to child only)
        // if send -1 print error message, destroy queue, and exit with error code 1
        if (msgsnd(message_id, (void *)&parent2child, MAX_MESSAGE_SIZE, 0) == -1){
            perror("Error: Parent unable to send message to the child through the message queue. Program exiting...\n\n");
            // destroy message queue
            msgctl(message_id, IPC_RMID, 0);
            // kill child process
            kill(child_pid, SIGKILL);

            exit(1);
        }
            // otherwise print success message
        else{
            printf("Parent successfully sent message to the child through the message queue.\n\n");
        }


        // Receiving response from the child using message id queue 2 (for child to parent only)
        // Reading from message queue
        if(msgrcv(message_id2, (void *)&child2parent, MAX_MESSAGE_SIZE, cp, 0) == -1){
            perror("Error: Parent unable to receive message from the child through the message queue. Program exiting...\n\n");
            // destroy message queue
            msgctl(message_id, IPC_RMID, 0);
            msgctl(message_id2, IPC_RMID, 0);

            // kill child process
            kill(child_pid, SIGKILL);
            exit(1);
        }else {
            printf("Parent Process: Recieved the following response from the child - \"%s\"\n\n", child2parent.message_text);
            printf("Program finished...\n");
        }
    }

    // wait for the child
    wait(NULL);

    // allow only the parent to destroy the queues
    if(child_pid != 0) {
        // destroy message queue at the end
        msgctl(message_id, IPC_RMID, 0);
        // destroy message queue at the end
        msgctl(message_id2, IPC_RMID, 0);
    }
    return 0;
}
