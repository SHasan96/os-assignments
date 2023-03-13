// Name: Saif Hasan
// CSC 460
// Date: 03/17/2023
// Dining Philosophers with sync

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N        5          // number of philosophers
#define LEFT     (i+N-1)%N  // i's left neighbor
#define RIGHT    (i+1)%N    // i's right neighbor
#define THINKING 0  
#define HUNGRY   1
#define EATING   2
#define DEAD     3 

// Semaphores
int semid, mutex;

// Shared memory variables
int shmid, *state;

// Variable to record the starting time
time_t start_time;

//prototypes
void philosopher(int i);
void think();
void eat();
void take_chopsticks(int i);
void put_chopsticks(int i);
void test(int i);
void print_states();

int main() {
    int  myid = -1; // Let the original process be marked using -1 so philosophers can be from 0-4 
   
    // Ask OS for shared memory
    shmid  =  shmget(IPC_PRIVATE, sizeof(int)*N, 0777);
    if (shmid == -1) {
        printf("Could not get shared memory.\n");
        return 0;
    }
    //Attach to the shared memory
    state = (int *) shmat(shmid, NULL, SHM_RND);
   
    semid = semget (IPC_PRIVATE, N, 0777);
    if (semid == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    mutex = semget (IPC_PRIVATE, 1, 0777);
    if (semid == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }

    //  Initialize semaphores.
    //  int semctl(int semid, int semnum, int cmd, ...);
    semctl(mutex, 0, SETVAL, 1); 
    int i;
    for (i=0; i<5; i++) {
        semctl(semid, i, SETVAL, 0);
    } 
   
    // Spawn the processes (5 philosophers excluding the original process)
    for (i=0; i<N; i++) {
        if (fork()) {
            break;
        }
        myid++;
    }    
    start_time = time(NULL); // Set the start time
   
    if (myid == -1) {
        print_states();
    } else {
        // Seed the random number generator with a different value for each process
        srand(time(NULL) + 100*getpid());
        philosopher(myid);
    }
       
    if (shmdt(state) == -1 ) {
        printf("shmgm: ERROR in detaching.\n");
    }      
    // Original process should clean up semaphores and shared memory
    wait();
    if (myid == -1) {
        if ((shmctl(shmid, IPC_RMID, NULL)) == -1) {
            printf("ERROR removing shmem.\n");
        }
        if ((semctl(mutex, 0, IPC_RMID, 0)) == -1) {
            printf("ERROR in removing semaphores.\n");
        }
        if ((semctl(semid, 0, IPC_RMID, 0)) == -1) {
            printf("ERROR in removing semaphores.\n");
        }
    }
    return 0;   
}

void philosopher(int i) {
    while (difftime(time(NULL), start_time) < 60) { // repeat until time is 60 seconds or more
        think();
        if (difftime(time(NULL), start_time) >= 60) {
            break;
        }
        take_chopsticks(i);
        eat();
        put_chopsticks(i);           
    }
    state[i] = DEAD;
}

void take_chopsticks(int i) {
    p(0, mutex);
    state[i] = HUNGRY;
    test(i);
    v(0, mutex);
    p(i, semid);
}

void put_chopsticks(int i) {
    p(0, mutex);
    state[i] = THINKING;
    test(LEFT);
    test(RIGHT);   
    v(0, mutex);
}

void test(int i) {
    if (state[i] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING) {
        state[i] = EATING;
        v(i, semid);
    }
}
	
void think() {
    int tt = generate_random_number(4, 10);
    sleep(tt);
}

void eat() {
    int et = generate_random_number(1, 3);
    sleep(et);
}

int generate_random_number(int min, int max) {
    return rand() % (max - min + 1) + min; 
}

void print_states() {
    int i;
    int t = 1; 
    while (1) { // keep printing states
        printf("%2d.  ", t);
        for(i=0; i<N; i++) {
            switch(state[i]) {
                case THINKING:
                    printf("%-15s", "thinking");
                    break;
                case HUNGRY:                
                    printf("%-15s", "hungry");
                    break;
                case EATING:
                    printf("%-15s", "eating");
                    break;
                case DEAD:
                    printf("%-15s", "dead"); 
                    break;
                default:
                    break;
            }
        }
        printf("\n");
        // stop when all philosophers are dead
        if (state[0] == DEAD && state[1] == DEAD && state[2] == DEAD &&
            state[3] == DEAD && state[4] == DEAD) {
            break;
        }
        sleep(1);  // delay 1 sec
        t++;
    }
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


