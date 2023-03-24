// Name: Saif Hasan
// CSC 460
// Date: 03/24/2023
// Dining Philosophers with BAD sync

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

#define LEFT             i           // i's left choptick (is same as philosopher number)
#define RIGHT            (i+1)%5     // i's right chopstick (is the next one)

// Semaphore
// [ NOTE: In Tanenbaum's solution the semaphores actually represented philosophers. 
//         Here they are for the chopsticks. ]
int chopsticks;

//prototypes
void philosopher(int i);
void think(int i);
void eat(int i);
void take_chopsticks(int i);
void put_chopsticks(int i);
void waste_time(int mag);
void print_tabs(int n);


int main() {
    int  myid = 0; 
      
    chopsticks = semget (IPC_PRIVATE, 5, 0777);
    if (chopsticks == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }

    //  Initialize semaphore
    int i;
    for (i=0; i<5; i++) {
        semctl(chopsticks, i, SETVAL, 1); // All chopsticks are available at the start (set them to 1)
    } 
   
    // Spawn the processes (4 more philosophers, 5 including the first process)
    for (i=1; i<5; i++) {
        if (fork()) {
            break;
        }
        myid++;
    }  
     
    // Seed the random number generator with a different value for each process
    srand(time(NULL) + 100*getpid());
    philosopher(myid);      

    // Nothing to clear since we will be at a deadlock
    return 0;   
}

void philosopher(int i) {
    while (1) { // loop infinitely
        think(i);
        take_chopsticks(i);
        eat(i);
        put_chopsticks(i);           
    }
}
	
void think(int i) {
    print_tabs(i);
    printf("%d %-8s\n", i, "THINKING");
    waste_time(1);
}

void eat(int i) {
    print_tabs(i);
    printf("%d %-8s\n", i, "EATING");
    waste_time(1);
}

void take_chopsticks(int i) {
    print_tabs(i);    
    printf("%d %-8s\n", i, "HUNGRY"); // get hungry and try to pick chopsticks
    p(LEFT, chopsticks);  // pick left chopstick
    waste_time(0);        // take some time
    p(RIGHT, chopsticks); // pick right chopstick  
}

void put_chopsticks(int i) {
    v(LEFT, chopsticks);  // put left chopstick down
    waste_time(0);        // take some time
    v(RIGHT, chopsticks); // put right chopstick down
}

void print_tabs(int n) {
    int i;
    for (i=0; i<n; i++) 
        printf("%-11s", "");
}

int generate_random_number(int min, int max) {
    return rand() % (max - min + 1) + min;     
}

void waste_time(int mag) {
    int i,j,k;
    int rnd = generate_random_number(10, 100);
    //printf("random num is %d\n", rnd);
    for (i=0; i<rnd; i++) {
        if (mag) { // for magnitude (1 or true) waste more time
            for(j=0; j<rnd; j++)
                for(k=0; k<rnd; k++); // simulate doing something   
        }
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


