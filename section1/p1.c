#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

/*
 * CPSC 457 Assignment 2
 * Section 1: Part 1
 *
 * Ryan Loi
 * Lecture: 02
 * Tutorial: T10
 */


/**
 * Main function for a utility that demonstrates the creation of child processes
 * @return 0 upon success, 1 upon fork error
 */
int main() {
    // forking to create a child
    int child_pid = fork();

    // check if fork had an error, exit (return) with error code 1 if this is the case
    if(child_pid == -1){
        perror("Error: Unable to fork... Program shutting down...\n\n");
        return 1;
    }

    // for the child process
    if(child_pid == 0){
        printf("Hello, I am the child process. My pid is: %d, and my parent's pid is: %d\n", getpid(), getppid());

    // parent process
    }else{
        printf("Hello I am the parent process. My pid is: %d, and my child's pid is: %d\n", getpid(), child_pid);
    }


    // waiting for child process to return
    wait(NULL);
    return 0;
}
