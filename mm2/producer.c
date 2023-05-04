// Name: Saif Hasan
// CSC 460
// Date: 05/03/23
// MemoryManager2 (Producer)
// PLEASE READ THE "test" script

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int rows;
    int cols;
    int bufsize;
    int timeslice; 
} ramDim_t;

typedef struct {
    char tag;
    int pid;
    int blocks;
    int time;
    int semid;
    int ramLoc;
    char state[10];
} reqRes_t;

int shmRamId; ramDim_t *dims;
int shmBufId; reqRes_t *buff;
int shmHead, *head;
int shmTail, *tail;
int shmEndId, *endSig;

int mutex, full, empty;

void validateArgs(int blocks, int time, int totalSize);
void readResources();
bool isInitialized();

#define HEAD head[0]
#define TAIL tail[0]
#define SIZE (dims->bufsize - 1)
#define END endSig[0]

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Invalid number of arguments.\n");
        exit(0);
    }
    // Assuming these will always be integers
    int blocks = atoi(argv[1]);
    int time = atoi(argv[2]);
 
    readResources();  
    validateArgs(blocks, time, dims->rows * dims->cols);
    
    // Once validated
    // create own semaphore for request    
    int mysem = semget (IPC_PRIVATE, 1, 0777);
    if (mysem == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(mysem, 0, SETVAL, 0);
    // Create requested resources struct variable
    reqRes_t req = {'\0', (int)getpid(), blocks, time, mysem, -1, "new"}; 

    printf("%d is requesting %d blocks of RAM for %d seconds.\n", getpid(), blocks, time);

    // Try to add this request to the shared bounded buffer
    if (!END) {
        p(0, empty);
        p(0, mutex); 
        // Write to buffer
        buff[HEAD] = req;
        HEAD = (HEAD+1) % SIZE;
        v(0, mutex);
        v(0, full); 
    }

    // Wait for request finished confirmation (from consumer) or end signal
    p(0, mysem);
    if (!END) printf("%d finished my request of %d blocks for %d seconds.\n", getpid(), blocks, time);

    // Detach shared mem
    if (shmdt(dims) == -1 ) 
        printf("shmgm: ERROR in detaching.\n");
    if (shmdt(buff) == -1 ) 
        printf("shmgm: ERROR in detaching.\n");
    if (shmdt(head) == -1 ) 
        printf("shmgm: ERROR in detaching.\n");  
    if (shmdt(tail) == -1 ) 
        printf("shmgm: ERROR in detaching.\n");  
     
    // Clear semaphore and die
    if ((semctl(mysem, 0, IPC_RMID, 0)) == -1) {
       printf("Error in removing semaphore.\n");
    }  
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

    fscanf(fp, "%d", &shmRamId);
    if ((dims = (ramDim_t*) shmat(shmRamId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
    }

    fscanf(fp, "%d", &shmBufId);
    if ((buff = (reqRes_t*) shmat(shmBufId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
    }
    
    fscanf(fp, "%d", &shmHead);
    if ((head = (int*) shmat(shmHead, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
    }

    fscanf(fp, "%d", &shmTail);
    if ((tail = (int*) shmat(shmTail, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
    }
   
    fscanf(fp, "%d\n", &shmEndId); 
    if ((endSig = (int*) shmat(shmEndId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
    }

    fscanf(fp, "%d\n", &mutex);
    fscanf(fp, "%d\n", &full);
    fscanf(fp, "%d\n", &empty);
  
    fclose(fp);
}

void validateArgs(int blocks, int time, int totalSize) {
    if (1>blocks || blocks>totalSize) {
        printf("Bad argument for blocks (should be within 1 and rows*cols)\n");
        exit(1);
    }
    if (1>time || time>30) {
        printf("Bad argument for time (should be within 1-30)\n");
        exit(1);
    }
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


// P function for semaphores
p(int s,int sem_id) {
    struct sembuf sops;
    sops.sem_num = s;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    if((semop(sem_id, &sops, 1)) == -1)
        printf("%s", "'P' error\n");
}

// V function for semaphores
v(int s, int sem_id) {
    struct sembuf sops;
    sops.sem_num = s;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    if((semop(sem_id, &sops, 1)) == -1)
        printf("%s","'V' error\n");
}

