// Name: Saif Hasan
// CSC 460
// Date: 02/01/2023
// Funky Forks - C program

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {        
    // If no argument is provided
    if (argc == 1) {
        printf("No argument provided!\n");
        exit(1);
    }
    
    // Assuming argument will always be an integer
    int N = atoi(argv[1]);
     
    // Validate range of N
    if (N > 5 || N < 0) {
        printf("Invalid range! N must be within 0-5 inclusive.\n");
        exit(1);    
    }

    printf("Gen\tPID  \tPPID \n");

    int C = 0; // Child/generation counter

    int i;
    if (N > 0) // No further process created if N=0
        C = 1;
    for (i=0; i<C; i++) {
        if (fork() == 0) {
            C++;
            if (C > N) break; 
            i = -1; // so that the next process' loop starts at i=0           
        } 
    }

    if (N > 0) // for starting count from 0th generation 
        C--; 
    printf("%d  \t%d\t%d\n", C, getpid(), getppid());
    sleep(1);
    return 0;
}

