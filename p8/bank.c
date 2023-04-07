// Name: Saif Hasan
// CSC 460
// Date: 04/01/23
// Bob’s Bank - Synchronization w/Sems

// **README -- When compiling we need to link the math library. 
//             cc bank.c -lm

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// Globals
int shmid, *shmem; // for shared memory
int mutex; // for semaphore

// Prototypes
int parseArgs(int argc, char** argv);
void readResources();
bool isInitialized();
void initBalance();
void showBalance();
void die();

#define BAL shmem[0]

int main(int argc, char** argv) {
    int opt = parseArgs(argc, argv);
    int myid = 0; // id number to keep track of the 16 processes
    int amount = (int) pow(2, myid);
    bool parent = true; // the initial processes are all parents
  
    // The cases are ordered as given in the assignment instructions
    switch (opt) {
        case 0: // Start things up, initialize shared memory and semaphore
            initBalance();
            break;
        case 1: // **This is where the simulation happens**
            semctl(mutex, 0, SETVAL, 1); // initialize mutex to 1

            int n = atoi(argv[1]); // loop counter for withdrawals/deposits
 
            // Spawn the processes (parents) (original process counts as one) 
            int i;
            for (i=0; i<15; i++) {
                if (fork()) {
                    break;
                }
                myid++;
                amount = (int) pow(2, myid); // amount = 2 ^ myid
            }
            
            // Spawn a child for the parents
            if (!fork()) {
                parent = false;
            }
            
            for(i=0; i<n; i++) {
                int oldBal = BAL;
                p(0, mutex);
                if (parent) {
                    BAL += amount;
                    printf("%d + %d = %d\n", oldBal, amount, BAL);
                } else {
                    BAL -= amount;
                    printf("\t\t%d - %d = %d\n", oldBal, amount, BAL);
                }
                v(0, mutex);
            }
            wait(); // for neat dispaly of prompt   
            break;
        case 2: // Show current balance
            showBalance();
            break;
        case 3: // Clean up and exit
            showBalance();
            die();
            break;
        default:
            printf("Invalid argument.\n");
            break;
    }
    wait(); // for neat display of prompt
    return 0;
}

// Initialize shared memory for balance.
void initBalance() {
    if (!isInitialized()) {
        shmid  =  shmget(IPC_PRIVATE, sizeof(int), 0777);
        if (shmid == -1) {
            printf("Could not get shared memory.\n");
            exit(1);
        }
        if ((shmem = (int*) shmat(shmid, NULL, SHM_RND)) == (void *)-1) {
            printf("Failed to attach shared memory.\n");
            exit(1);
        }
        BAL = 1000; // Initialize balance to 1000
        // Write shmid into a file (we assume that this file will not be tampered with externally)
        FILE *fp;
        fp = fopen("info.txt", "w"); 
        if (fp == NULL) {
            printf("Error creating file.\n");
            exit(1);
        }
        fprintf(fp, "%d\n", shmid);

        // Might as well make the semaphore here
        mutex = semget (IPC_PRIVATE, 1, 0777);
        if (mutex == -1) {
            printf("Failed to get semaphore.\n");
            exit(1);
        }
        fprintf(fp, "%d\n", mutex);
        fclose(fp); 
    } else {
        printf("The system is already ready for simulation.\n");
    }
}

// Show the current balance (the "balance" argument)
void showBalance() {
    readResources();
    printf("Current balance: %d\n", BAL); 
}

// Clean up shared resources and exit
void die() {
   readResources();
   if ((shmctl(shmid, IPC_RMID, NULL)) == -1) {
       printf("Error removing shmem.\n");
       exit(1);
   }
   if ((semctl(mutex, 0, IPC_RMID, 0)) == -1) {
       printf("Error in removing semaphore.\n");
   }
   system("rm info.txt");
   printf("Shutting down...\n"); 
   exit(0);   
}

// Read shmid and mutex from file
void readResources() {
    if (!isInitialized()) {
        printf("The simulation has not been initialized!\n");
        exit(1);    
    }
    FILE *fp;
    fp = fopen("info.txt", "r");
    if (fp == NULL) {
        printf("Error reading file.\n");
        exit(1);
    }
    fscanf(fp, "%d", &shmid);
    if ((shmem = (int*) shmat(shmid, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    fscanf(fp, "%d", &mutex);
    fclose(fp);
}

// Parse the command line arguments and return an integer from 0-3 to indicate 
// one of the four possible operations
int parseArgs(int argc, char** argv) {
    if (argc == 1) { // no args
        return 0;
    } 
    if (strcmp(argv[1], "balance") == 0) { // balance
        return 2;
    }
    if (strcmp(argv[1], "die") == 0) { // die
        return 3;
    }
    // Work on the integer arg
    char *ptr;
    long n = strtol(argv[1], &ptr, 10);
    if (*ptr != '\0' || ptr == argv[1]) {
        return -1; // was not an integer
    } else {
        if (n<1 || n>100) {
            printf("Integer argument should be within 1-100.\n");
            exit(1);
        }
        readResources(); // read the shared memory to check if its ready before proceeding
        return 1; // valid integer
    }
    return -1;
}

// Check if file exists.
bool isInitialized() {
    FILE *fp;
    fp = fopen("info.txt", "r");
    if (fp == NULL) {
        return false;
    }
    fclose(fp);
    return true;
}

// P and V functions for semaphores
p(int s,int sem_id) {
    struct sembuf sops;
    sops.sem_num = s;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    if((semop(sem_id, &sops, 1)) == -1)
        printf("%s", "'P' error\n");
}

v(int s, int sem_id) {
    struct sembuf sops;
    sops.sem_num = s;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    if((semop(sem_id, &sops, 1)) == -1)
        printf("%s","'V' error\n");
}


