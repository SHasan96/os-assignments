// Name: Saif Hasan
// CSC 460
// Date: 04/17/23
// MemoryManager1 (Consumer)
// PLEASE READ THE "test" script

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


/** Globals **/

/* Structs */

// for requested resources
typedef struct {
    char tag;
    int pid;
    int blocks;
    int time;
    int semid;
    bool inRam;
} reqRes_t;

// for simulated RAM dimension
typedef struct {
    int rows;
    int cols;
    int bufsize; // we need this for reading/writing into buffer
} ramDim_t;

// for free slot in ram
typedef struct {
    int row;
    int col;
} freeSlot_t;

/*  Shared memories */
// for simulated RAM blocks
int shmRamId; ramDim_t *simRam; 

// for the buffer
int shmBufId; reqRes_t *buff;
int shmHead, *head;
int shmTail, *tail;

// for a terminator/ending signal
int shmEndId, *endSig;

// for an array of requests (acts as a queue)
int shmQueueId; reqRes_t *q; 

/* Semaphores */
int mutex, full, empty;

// Prototypes
bool isInitialized();
void initResources(char** argv);
void initSharedMems(int rows, int cols, int bufsize);
void initSems(int bufsize);
void writeResourceInfo();
void displayReq(reqRes_t r);
void addReq(reqRes_t req);
void remReq(reqRes_t req);
freeSlot_t findFirstAvail(int rows, int cols, char arr[*][*], int size);
void fillRam(int rows, int cols, char arr[*][*], char tag, int size, freeSlot_t slot);
void clearFromRam(int rows, int cols, char arr[*][*], char tag);
void displayRam(int rows, int cols, char [*][*]);
void clearShm();
void clearSem();
void relieveAllProcs();
void displayProcList();
bool isQueueEmpty();

#define DIMS simRam[0]
#define END endSig[0]

// for the buffer
#define HEAD head[0]
#define TAIL tail[0]
#define SIZE (simRam[0].bufsize - 1)

// for the processes based on the tasks they will perform
#define ORIGINAL 0
#define SYNCHRONIZER 1
#define MANAGER 2
#define DISPLAYER 3


int main(int argc, char** argv) {
    // Setup and initialize shared memories, semaphores. Write them into a file.
    if (argc != 4) {
        printf("Invalid number of arguments.\n");
        exit(1);
    }
    initResources(argv);  
 
    int myId = ORIGINAL;

    // A semaphore for RAM management only needed in consumer
    int manager = semget (IPC_PRIVATE, 1, 0777);
    if (manager == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(manager, 0, SETVAL, 1); // goes 1st, set val as 1

    // Another semaphore for the consumer
    int displayer = semget (IPC_PRIVATE, 1, 0777);
    if (displayer == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(displayer, 0, SETVAL, 0); // goes after manager, set val as 0

    // Fork off process to do different tasks
    int i;
    for(i=0; i<3; i++) {
       if(fork()) {
           break;
        }
        myId++;
    }
  
    // Do different tasks based on myId
    switch(myId) { 
        case(SYNCHRONIZER): { 
            int tagger = 0;
            while(!END) {
                p(0, full);
                p(0, mutex);
                // Read from buffer
                reqRes_t req = buff[TAIL];
                TAIL = (TAIL+1)%SIZE;
                tagger = (tagger + 1) % 25;
                req.tag = 'A' + tagger - 1;
                if (!END) addReq(req); // add to array(so called queue)
                v(0, mutex);
                v(0, empty);
            }       
            break;
        }        
        case(MANAGER): {
            sleep(1); // give a second for synchronizer to start filling bounded buffer
            while(!END) {
                p(0, manager); // start before displayer
                if(END) break;
                // For procs in RAM decrement time and set inRAM to false if time becomes 0
                // Should do nothing in 1st iteration because inRam is initially false 
                int i;
                for (i=0; i<26; i++) {
                    if (q[i].inRam) { // if proc in RAM decrement time
                        q[i].time--;
                        if (q[i].time<=0) {
                            q[i].inRam = false;
                            v(0, q[i].semid);
                        }
                    }
                }       
                sleep(1);
                if (!isQueueEmpty() && !END) displayProcList();  // display procs in non-empty array                               
                v(0, displayer); // let displayer go now
            }
            break;
        }
        case(DISPLAYER): {
            char ramArr[DIMS.rows][DIMS.cols];
            int i, j;
            for (i = 0; i < DIMS.rows; i++) {
                for (j = 0; j < DIMS.cols; j++) {
                    ramArr[i][j] = '.';
                }
            }
                
            while(!END) {
                p(0, displayer); // starts after manager
                // Find empty slot for procs not in RAM yet
                int k;
                for(k=0; k<26; k++) {
                    // For procs not inRAM but time requirement is fulfilled
                    // Should do nothing for 1st iteration because no process got time in RAM
                    if (q[k].pid != -1 && !q[k].inRam && q[k].time<=0) { // if a proc met its time requirement
                        clearFromRam(DIMS.rows, DIMS.cols, ramArr, q[k].tag); 
                        remReq(q[k]);
                    }
                    // For procs not inRAM with time requirement not met
                    if(q[k].pid != -1 && !q[k].inRam && q[k].time>0) {
                        freeSlot_t slot = findFirstAvail(DIMS.rows, DIMS.cols, ramArr, q[k].blocks);
                        if (slot.row != -1) { // if an empty slot was found for a proc not in ram
                            q[k].inRam = true;
                            //printf("Slot found at (%d,%d) for %c\n", slot.row, slot.col, q[k].tag); 
                            fillRam(DIMS.rows, DIMS.cols, ramArr, q[k].tag, q[k].blocks, slot);
                        }
                    }                    
                }               
                if (!isQueueEmpty() && !END) displayRam(DIMS.rows, DIMS.cols, ramArr); // display updated ram
                if(END) break;
                v(0, manager); // back to manager
            }
            v(0, manager);                                 
            break;
        }            
        default:
            break;
    }
  
    // The original process will clear things up
    wait();
    if (myId == ORIGINAL && END) { 
        relieveAllProcs();
        // Detach and clear shared mems
        if (shmdt(simRam) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmRamId);       
        if (shmdt(buff) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmBufId);
        if (shmdt(head) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmHead);
        if (shmdt(tail) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmTail);
        if (shmdt(endSig) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmEndId);
        if (shmdt(q) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmQueueId);
        // Clear sems
        clearSem(manager);
        clearSem(displayer);
        clearSem(mutex);
        clearSem(full);
        clearSem(empty);
        // Remove file
        system("rm resources.txt");
    }
    return 0;    
}

// Initialize necessary shared resources
void initResources(char** argv) {
    // Only one consumer can run at a time
    if (isInitialized()) {
        printf("An instance of Consumer is already running.\n");
        exit(0);
    }
    // Not checking/validationg arguments, assuming they will be always integers
    int rows = atoi(argv[1]);
    if (1>rows || rows>20) {
        printf("Bad argument for rows (should be within 1-20).\n");
        exit(1);
    }
    int cols = atoi(argv[2]);
    if (1>cols || cols>50) {
        printf("Bad argument for columns (should be within 1-50).\n");
        exit(1);
    }
    int bufsize = atoi(argv[3]);
    if (1>bufsize || bufsize>26) {
        printf("Bad argument for buffer size (should be within 1-26).\n");
        exit(1);
    }

    initSharedMems(rows, cols, bufsize);
}

// Initialize shared memories
void initSharedMems(int rows, int cols, int bufsize) {
    // Store RAM dimensions in the struct  
    ramDim_t dims = {rows, cols, bufsize};
  
    // Create shared memories and attach to them
    // For the simulated RAM 
    shmRamId  =  shmget(IPC_PRIVATE, sizeof(ramDim_t), 0777);
    if (shmRamId == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((simRam = (ramDim_t*) shmat(shmRamId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    DIMS = dims;

    // For the buffer
    shmBufId  =  shmget(IPC_PRIVATE, sizeof(reqRes_t)*bufsize, 0777);
    if (shmBufId == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((buff = (reqRes_t*) shmat(shmBufId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }

    // For the head of the buffer
    shmHead  =  shmget(IPC_PRIVATE, sizeof(int), 0777);
    if (shmHead == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((head = (int*) shmat(shmHead, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    HEAD = 0;
   
    // For the tail/rear of the buffer
    shmTail  =  shmget(IPC_PRIVATE, sizeof(int), 0777);
    if (shmTail == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((tail = (int*) shmat(shmTail, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    TAIL = 0;
    // For the ending signal
    shmEndId  =  shmget(IPC_PRIVATE, sizeof(int), 0777);
    if (shmEndId == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((endSig = (int*) shmat(shmEndId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    END = 0;
 
    // For the queue (actually an array) of requests
    shmQueueId  =  shmget(IPC_PRIVATE, sizeof(reqRes_t)*26, 0777);
    if (shmQueueId == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((q = (reqRes_t*) shmat(shmQueueId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
   
    // Initialize it with dummy request (placeholder for null)
    reqRes_t dummy = {'\0', -1, -1, -1, -1, false};
    int i;
    for (i=0; i<26; i++) {
        q[i] = dummy;
    }
    
    initSems(bufsize);
}

// Initialize semaphores
void initSems(int bufsize) {
    // Mutex 
    mutex = semget (IPC_PRIVATE, 1, 0777);
    if (mutex == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(mutex, 0, SETVAL, 1);
    
    // Full 
    full = semget (IPC_PRIVATE, 1, 0777);
    if (full == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(full, 0, SETVAL, 0); // full = 0
    
    // Empty
    empty = semget (IPC_PRIVATE, 1, 0777);
    if (empty == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(empty, 0, SETVAL, bufsize); // empty = buffer size
   
    writeResourceInfo();
}

// Write shared memory ids and semaphore ids into a file
// (we assume that this file will not be tampered with externally)
void writeResourceInfo() {
    FILE *fp;
    fp = fopen("resources.txt", "w"); 
    if (fp == NULL) {
        printf("Error creating file.\n");
        exit(1);
    }

    // Write ids in the this order in separate lines
    fprintf(fp, "%d\n", shmRamId);  // RAM shm
    fprintf(fp, "%d\n", shmBufId);  // Buffer shm
    fprintf(fp, "%d\n", shmHead);   // Head of buffer shm
    fprintf(fp, "%d\n", shmTail);   // Tail of buffer shm
    fprintf(fp, "%d\n", shmEndId);  // EndSignal shm
    fprintf(fp, "%d\n", mutex);     // Mutex sem
    fprintf(fp, "%d\n", full);      // Full sem
    fprintf(fp, "%d\n", empty);     // Empty sem
   
    fclose(fp);
}

void displayReq(reqRes_t r) {
    printf("%c. %6d %4d %3d\n", r.tag, r.pid, r.blocks, r.time);
}

// Check if file exists.
bool isInitialized() {
    FILE *fp;
    fp = fopen("resources.txt", "r");
    if (fp == NULL) {
        return false;
    }
    fclose(fp);
    return true;
}

// Add request to an empty array slot
void addReq(reqRes_t req) {
   int i;
   // Find an empty slot and put the request in 
   for(i=0; i<26; i++) {
       if (q[i].pid == -1) {
           q[i] = req; 
           break;
       } 
   }
}

// Remove request from array
void remReq(reqRes_t req) {
   reqRes_t dummy = {'\0', -1, -1, -1, -1, false};
   int i=0;
   for(i=0; i<26; i++) {
       if(q[i].pid == req.pid) {
           q[i] = dummy;
           break;
       }
   }
}

bool isQueueEmpty() {
   int i=0;
   for(i=0; i<26; i++) {
       if(q[i].pid != -1 && q[i].time>0) {
           return false;
       }
   }
   return true;
} 

// Find first available contiguous blocks
freeSlot_t findFirstAvail(int rows, int cols, char arr[rows][cols], int size) {
    int i, j;
    int im = -1, jm = -1; // marker for start of free blocks
    int free = 0;

    freeSlot_t slot = {im, jm};
    for(i=0; i<rows; i++) {
        for(j=0; j<cols; j++) {
            if (arr[i][j] == '.') {
                if(im == -1) {
                    im = i; 
                    jm = j;
                }   
                free++;
            } else {
                im = -1;
                jm = -1;
                free = 0;
            }
            if (free == size) {
                slot.row = im;
                slot.col = jm; 
                return slot;
            }
        }
    }
    return slot;
}

// Fill the RAM blocks
void fillRam(int rows, int cols, char arr[rows][cols], char tag, int size, freeSlot_t slot) {
    int i, j;
    int filled = 0;
    for(i=slot.row; filled<size; i++) {
        for(j=slot.col; filled<size; j++) {
            arr[i][j] = tag;
            filled++;
        }
    } 
}

// Clear a completed proc from RAM
void clearFromRam(int rows, int cols, char arr[rows][cols], char tag) {
    int i, j;
    for(i=0; i<rows; i++) {
        for(j=0; j<cols; j++) {
            if (arr[i][j] == tag) {
                arr[i][j] = '.';
            }
        }
    }
}

// Display a visualization of RAM
void displayRam(int rows, int cols, char arr[rows][cols]) {
    int i, j;
    for (j=0; j<cols+2; j++) {
        printf("%c", '*');
    }
    printf("\n");

    for(i=0; i<rows; i++) {
        for(j=-1; j<cols+1; j++) {
            if (j==-1 || j==cols) printf("%c", '*');
            else printf("%c", arr[i][j]);
        }
        printf("\n");   
    }

    for (j=0; j<cols+2; j++) {
        printf("%c", '*');
    }
    printf("\n");
}

// Forcefully stop all producers
void relieveAllProcs() {
   int i;
   for(i=0; i<26; i++) {
       if (q[i].pid != -1) {
           v(0, q[i].semid);
       }
   }
}

void displayProcList() {
    int i;
    printf("\nID thePID Size Sec\n");
    for(i=0; i<26; i++) {
        if(q[i].pid != -1 && q[i].time>0) // don't show if time requirement is met
            displayReq(q[i]);
    }
}

// Clear shared memory
void clearShm(int shmid) {
    if ((shmctl(shmid, IPC_RMID, NULL)) == -1)
        printf("ERROR removing shmem.\n");
}


// Clear semaphore
void clearSem(int semid) {
    if ((semctl(semid, 0, IPC_RMID, 0)) == -1) {
        printf("ERROR in removing semaphores.\n");
    }
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


