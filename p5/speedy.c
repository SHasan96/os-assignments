// Name: Saif Hasan
// CSC 460
// Date: 02/26/23
// Super Speedy Synch - Fork and Semaphores

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

#define FIRST 0 // first process
  
int main(int argc, char **argv) {
    if (argc == 1) {
        printf("An integer argument within 1-26 (inclusive) must be provided.\n");
        exit(1);
    }

    int n = atoi(argv[1]);
    if (n<1 || n>26) {
        printf("Integer arguments out of range, must be within 1-26 (inclusive).\n");
        exit(1);
    }

    int  semid, myid = FIRST;

    //  Ask OS for semaphores.
    semid = semget (IPC_PRIVATE, n, 0777);

    //  See if you got the request.
    if (semid == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }

    //  Initialize your sems.
    //  int semctl(int semid, int semnum, int cmd, ...);
    semctl(semid, FIRST, SETVAL, 1); // first process gets to go first, so its val is 1
    // other processes (from index 1) must be initialized with 0 val
    int i;
    for (i=1; i<n; i++) {
        semctl(semid, i, SETVAL, 0);
    }
   
    // Spawn the processes (remember that we already have 1!)
    for (i=1; i<n; i++) {
        if (fork()) {
            break;
        }
        myid++;
    }
    
    // Loop 5 times to print characters
    for (i=0; i<5; i++) {
        p(myid, semid);
        printf("%c:%d\n", 'A'+myid, getpid());
        v((myid+1)%n, semid);
    }

    wait();
    if (myid == FIRST) {
        if ((semctl(semid, 0, IPC_RMID, 0)) == -1) {
            printf("ERROR in removing semaphores.\n");
        }
    }
    return 0;   
}

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


