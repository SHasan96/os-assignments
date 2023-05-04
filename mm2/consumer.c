// Name: Saif Hasan
// CSC 460
// Date: 05/03/23
// MemoryManager2 (Consumer)
// PLEASE READ THE "test" script

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int ramLoc;
    char state[10];
} reqRes_t;

// for simulated RAM dimension
typedef struct {
    int rows;
    int cols;
    int bufsize;
} ramDim_t;

// for free slot in ram
typedef struct {
    int row;
    int col;
    int loc;
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

// for an array of requests (acts as a pcb)
int shmQueueId; reqRes_t *pcb;
int shmPcbTrackerId, *tracker;

// for the ready queue
int readyQueueId; reqRes_t *readyQueue;
int shmFront, *front;
int shmRear, *rear;

// for keeping track of cpu activity
int shmCpuId, *activeCPU;

/* Semaphores */
int mutex, full, empty;

/* Variable */
int timeslice;


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
void removeFromReadyQueue(int ind);


#define DIMS simRam[0]
#define END endSig[0]
#define CPU_ACTIVE activeCPU[0]
#define TRACKER tracker[0]

// for the buffer
#define HEAD head[0]
#define TAIL tail[0]
#define SIZE (simRam[0].bufsize - 1)
// for ready queue
#define FRONT front[0]
#define REAR  rear[0]

// for the processes based on the tasks they will perform
#define ORIGINAL     0
#define SYNCHRONIZER 1
#define MANAGER      2
#define DISPATCHER   3
#define CPU          4


int main(int argc, char** argv) {
    // Setup and initialize shared memories, semaphores. Write them into a file.
    if (argc != 5) {
        printf("Invalid number of arguments.\n");
        exit(1);
    }
    initResources(argv);  

    int myId = ORIGINAL;
    
    // A semaphore for dispatcher
    int dispatcher = semget (IPC_PRIVATE, 1, 0777);
    if (dispatcher == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(dispatcher, 0, SETVAL, 0);

    // A semaphore for cpu
    int cpu = semget (IPC_PRIVATE, 1, 0777);
    if (cpu == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(cpu, 0, SETVAL, 0);

    // A semaphore for RAM management only needed in consumer
    int manager = semget (IPC_PRIVATE, 1, 0777);
    if (manager == -1) {
        printf("Failed to get semaphore.\n");
        exit(1);
    }
    semctl(manager, 0, SETVAL, 1); // goes 1st, set val as 1

    // Fork off process to do different tasks
    int i;
    for(i=0; i<4; i++) {
       if(fork()) 
           break;
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
                if (!END) addReq(req); // add to pcb
                v(0, mutex);
                v(0, empty);
            }  
            wait();
            //printf("SYNCHRONIZER ending\n");  
            break;
        }        
        case(MANAGER): {
            char ramArr[DIMS.rows][DIMS.cols];
            int i, j;
            for (i = 0; i < DIMS.rows; i++) {
                for (j = 0; j < DIMS.cols; j++) {
                    ramArr[i][j] = '.';
                }
            }
            while(!END) {
                p(0, manager); // start before dispatcher
                sleep(1);
                if(END) {
                    v(0, dispatcher);
                    break;
                }
                if (!isQueueEmpty() && !END) displayProcList();  // display procs in non-empty array
                if (!isQueueEmpty() && !END) displayRam(DIMS.rows, DIMS.cols, ramArr);
                int i;
                for (i=0; i<TRACKER; i++) {
                    if (pcb[i].pid != -1) { // an actual process in pcb
                        if (strcmp(pcb[i].state, "new") == 0 || strcmp(pcb[i].state, "suspended") == 0) { 
                            freeSlot_t slot = findFirstAvail(DIMS.rows, DIMS.cols, ramArr, pcb[i].blocks); // find slot in ram
                            if(slot.row != -1) { // slot found? put into ram and mark state as ready
                                fillRam(DIMS.rows, DIMS.cols, ramArr, pcb[i].tag, pcb[i].blocks, slot);
                                pcb[i].ramLoc = slot.loc;
                                strcpy(pcb[i].state, "ready");
                                // add to the ready queue
                                readyQueue[FRONT] = pcb[i];
                                FRONT = (FRONT+1) % 99;
                            } else {
                                strcpy(pcb[i].state, "suspended"); // if no spot found, mark as suspended
                            }
                        }
                        if (strcmp(pcb[i].state, "terminated") == 0) { // terminated processes are cleared
                            clearFromRam(DIMS.rows, DIMS.cols, ramArr, pcb[i].tag);
                            v(0, pcb[i].semid);
                            remReq(pcb[i]);
                        }
                        if (strcmp(pcb[i].state, "run") == 0) {
                            pcb[i].time--;
                            if(pcb[i].time <= 0) strcpy(pcb[i].state, "terminated");
                        }
                    }                   
                }                                
                if (END) {
                    v(0,dispatcher);
                    break;   
                }
                else if(!CPU_ACTIVE) v(0, dispatcher); // let dispatcher go now
                else v(0, manager);
            }
            wait();
            //printf("MANAGER ending\n");            
            break;
        }
        case(DISPATCHER): 
            while(!END) {
                p(0, dispatcher);
                if (END) {
                    v(0,cpu);
                    break;
                }
                CPU_ACTIVE = 0;
                if (FRONT != REAR && FRONT != 0) {
                    int i; 
                    if(strcmp(readyQueue[REAR].state, "terminated") == 0) { // remove any terminated proc from ready queue
                        /*for(i=0; i<100; i++) {
                            if(readyQueue[REAR].pid == pcb[i].pid) {
                                strcpy(pcb[i].state, "terminated");
                                pcb[i].time = 0;
                            }
                        }*/
                        removeFromReadyQueue(REAR);
                    } 
                    if(strcmp(readyQueue[REAR].state, "run") == 0) { // time to stop running proc temporarily
                        strcpy(readyQueue[REAR].state, "ready");
                        for(i=0; i<TRACKER; i++) {
                            if(readyQueue[REAR].pid == pcb[i].pid) {
                                strcpy(pcb[i].state, "ready");
                                break;
                            }
                        }
                        REAR = (REAR+1)%FRONT;                  
                    } 
                    if(strcmp(readyQueue[REAR].state, "ready") == 0) { // allow the next proc in line to run
                        strcpy(readyQueue[REAR].state, "run");
                        for(i=0; i<TRACKER; i++) {
                            if(readyQueue[REAR].pid == pcb[i].pid) {
                                strcpy(pcb[i].state, "run");
                                break;
                            }
                        }
                        v(0, cpu);
                    }
                    if (END) {
                        v(0, cpu);
                        break;    
                    } 
                }           
            }
            wait();  
            //printf("DISPATCHER ending\n");                 
            break;
        case(CPU):
            while(!END) {
                p(0, cpu);
                CPU_ACTIVE = 1; 
                if (END) {
                    v(0, manager);
                    break;
                } else if(readyQueue[REAR].time > timeslice) {
                    v(0, manager);
                    readyQueue[REAR].time -= timeslice;
                    sleep(timeslice);
                } else {
                    v(0, manager);
                    strcpy(readyQueue[REAR].state, "terminated");
                    sleep(readyQueue[REAR].time);            
                }
                CPU_ACTIVE = 0;
                if (END) {
                    v(0, manager);
                    break;
                }
            }
            wait();
            //printf("CPU ending\n");
            break;          
        default:           
            break;
    } 
   
    //printf("MYID: %d finsihing.\n", myId);
    wait();
    //sleep(timeslice);
    if (myId == ORIGINAL && END) {
        //printf("ORIGINAL cleaning up before ending\n");
        v(0, manager);
        v(0, dispatcher);
        v(0, cpu);
        sleep(2); 
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
        if (shmdt(pcb) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmQueueId);
        if (shmdt(readyQueue) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(readyQueueId);
        if (shmdt(front) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmFront);
        if (shmdt(rear) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmRear);
        if (shmdt(activeCPU) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmCpuId);
        if (shmdt(tracker) == -1 ) printf("shmgm: ERROR in detaching.\n");
        clearShm(shmPcbTrackerId);
        // Clear sems
        clearSem(mutex);
        clearSem(full);
        clearSem(empty);
        clearSem(manager);
        clearSem(dispatcher);
        clearSem(cpu);
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
   
    timeslice = atoi(argv[4]);
    if (1>timeslice || timeslice>10) {
        printf("Bad argument for buffer size (should be within 1-10).\n");
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
 
    // For the queue (actually an array) of requests/ hardcode it to 100, let's say dmp is 100
    shmQueueId  =  shmget(IPC_PRIVATE, sizeof(reqRes_t)*100, 0777);
    if (shmQueueId == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((pcb = (reqRes_t*) shmat(shmQueueId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
   
    // Initialize it with dummy request (placeholder for null)
    reqRes_t dummy = {'\0', -1, -1, -1, -1, -1, "\0"};
    int i;
    for (i=0; i<100; i++) {
        pcb[i] = dummy;
    }
 
    // New ready queue for dispatcher   
    readyQueueId =  shmget(IPC_PRIVATE, sizeof(reqRes_t)*100, 0777); // we assumed dmp to be 100 (so max 100 processes can share cpu)
    if (readyQueueId == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((readyQueue = (reqRes_t*) shmat(readyQueueId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }

    shmFront  =  shmget(IPC_PRIVATE, sizeof(int), 0777);
    if (shmFront == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }
    if ((front = (int*) shmat(shmFront, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    FRONT = 0;

    shmRear  =  shmget(IPC_PRIVATE, sizeof(int), 0777);
    if (shmRear == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }

    if ((rear = (int*) shmat(shmRear, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    REAR = 0;
    
    shmCpuId  =  shmget(IPC_PRIVATE, sizeof(int), 0777);
    if (shmCpuId == -1) {
        printf("Could not get shared memory.\n");
        exit(1);
    }

    if ((activeCPU = (int*) shmat(shmCpuId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    CPU_ACTIVE = 0;

    shmPcbTrackerId  =  shmget(IPC_PRIVATE, sizeof(int), 0777);
    if ((tracker = (int*) shmat(shmPcbTrackerId, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    TRACKER = 0;
     
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

// Display process info (NEEDS UPDATE)!!!!!!!!!!!!!!1
void displayReq(reqRes_t r) {
    if (r.ramLoc != -1) {
        printf("%c. %-6d %-11s%-4d %-4d %-3d\n", r.tag, r.pid, r.state, r.ramLoc, r.blocks, r.time);
    } else {
        printf("%c. %-6d %-11s%-4s %-4d %-3d\n", r.tag, r.pid, r.state, "", r.blocks, r.time);
    }
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
   pcb[TRACKER] = req;
   TRACKER = (TRACKER+1)%99;
}

// Remove request from array
void remReq(reqRes_t req) {
   reqRes_t dummy = {'\0', -1, -1, -1, -1, -1, "\0"};
   int i=0;
   for(i=0; i<TRACKER; i++) {
       if(pcb[i].pid == req.pid) {
           pcb[i] = dummy;
           break;
       }
   }
}

bool isQueueEmpty() {
   int i=0;
   for(i=0; i<TRACKER; i++) {
       if(pcb[i].pid != -1 && pcb[i].time>0) {
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

    freeSlot_t slot = {im, jm, -1};
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
                slot.loc = im*cols + jm;
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
   for(i=0; i<TRACKER; i++) {
       if (pcb[i].pid != -1) {
           v(0, pcb[i].semid);
       }
   }
}

void displayProcList() {
    int i;
    printf("\nID thePID State      Loc Size Sec\n");
    for(i=0; i<TRACKER; i++) {
        if(pcb[i].pid != -1) 
            displayReq(pcb[i]);
    }
}

void removeFromReadyQueue(int ind) {
    int i;
    for(i=ind+1; i<FRONT; i++) {
        readyQueue[i-1] = readyQueue[i];
    } 
    FRONT--;
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


