// Name: Saif Hasan
// CSC 460
// Date: 2/17/2023
// Medium Speed Synch - Fork and Shared Mem

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>

#define TURN shmem[0]

main(int argc, char **argv) { 
    //Assuming provided argument is an integer
    int n;
    if (argc < 2) {
        printf("Please provide an integer argument within 1-26 (inclusive).\n");
        return 0;
    } else {
        n = atoi(argv[1]);
    }
    
    // Validate the argument
    if (n<1 || n>26) {
        printf("Invalid argument! The argument must be an integer within 1-26 (inclusive).\n");
        return 0;
    }

    //Note first process id, it will be used to free shared memory
    int firstid = getpid();

    // Variables for shared memory
    int shmid; 
    char *shmem;    
    
    // Get Shared Memory and ID 
    shmid  =  shmget(IPC_PRIVATE, sizeof(char), 0777);
    if (shmid == -1) {
        printf("Could not get shared memory.\n");
        return 0;
    }

    // Attach to the shared memory  
    shmem = (char *) shmat(shmid, NULL, SHM_RND);

    // Initialize the shared memory 
    TURN = 'A';

    // Letters to identify current and last processes 
    char  myid = 'A';
    char  lastid = 'A' + (n-1);
   
    // Spawn all the processes
    int i; 
    for (i = 1; i < n; i++) {
        if (fork() > 0) 
            break; // send parent on to Body
         myid++;
    }

    // BODY: Loop 5 times
    for (i = 0; i < 5; i++) {
        while(TURN != myid);  // busy, wait for child 

        printf("%c: %d\n", myid, getpid());

        if (TURN == lastid) 
            TURN = 'A';
        else 
            TURN++; 
    }
          
    // Detach and clean-up the shared memory  
    if (shmdt(shmem) == -1 ) 
        printf("shmgm: ERROR in detaching.\n");
    
    if (firstid == getpid())         // ONLY need one process to do this
        if ((shmctl(shmid, IPC_RMID, NULL)) == -1)
            printf("ERROR removing shmem.\n");

    wait();
    return 0;
}
