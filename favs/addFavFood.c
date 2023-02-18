// Name: Saif Hasan
// CSC 460
// Date: 02/17/23
// Favs List (Add/Change favFood)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char name[20];
    int favNum;
    char favFood[30];
} bbStruct_t;

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("No argument provided!\n"); 
        exit(1);       
    } 
   
    FILE *fopen(), *fp;
    int shmid;
    if ((fp = fopen("/pub/csc460/bb/BBID.txt","r")) == NULL) {
        printf("Failed to open file for reading.\n");
        exit(1);
    }
    fscanf(fp,"%d",&shmid);

    bbStruct_t *shmem;
    if ((shmem = (bbStruct_t *) shmat(shmid, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
 
    // I am number 4 (index 3) on the list.
    shmem += 3;
    // favFood size is 30 including null terminator,
    // so we can have upto 29 other characters.
    // Note: The last spot must be left for the null terminator.
    strncpy(shmem->favFood, argv[1], 29); 
    return 0;
}
