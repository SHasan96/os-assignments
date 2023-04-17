// Name: Saif Hasan
// CSC 460
// Date: 04/17/23
// MemoryManager1 (Shutdown)
// PLEASE READ THE "test" script

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// semaphores
int mutex, full, empty;

//shared mem
int shmEndId, *endSig;

void readResources();
bool isInitialized();

int main() {
   readResources();    
   endSig[0] = 1; //set end flag to 1
   // let the consumer continue to its death
   v(0, mutex);  
   v(0, full);

   if (shmdt(endSig) == -1 ) 
        printf("shmgm: ERROR in detaching.\n");
   
   return 0;
}

void readResources() {
    if (!isInitialized()) {
        printf("No Consumer instance is running.\n");
        exit(1);
    }

    FILE *fp;
    fp = fopen("resources.txt", "r");
    if (fp == NULL) {
        printf("Error reading file.\n");
        exit(1);
    }

    int i;
    for(i=0; i<5; i++) {
        fscanf(fp, "%d\n", &shmEndId); 
    }

    if ((endSig = (int*) shmat(shmEndId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
    }
       
    fscanf(fp, "%d\n", &mutex);
    fscanf(fp, "%d\n", &full);
    fclose(fp);
}

bool isInitialized() {
    FILE *fp;
    fp = fopen("resources.txt", "r");
    if (fp == NULL) {
        return false;
    }
    fclose(fp);
    return true;
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
 
