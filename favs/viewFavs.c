// Name: Saif Hasan
// CSC 460
// Date: 02/17/23
// Favs List (View the list)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    char name[20];
    int favNum;
    char favFood[30];
} bbStruct_t;

int main() {
    FILE *fopen(), *fp;
    int shmid; 
    //Read from /pub/csc460/bb/BBID.txt
    if ((fp = fopen("/pub/csc460/bb/BBID.txt","r")) == NULL) {
        printf("Failed to open file for reading.\n");
        exit(1);
    } 
    fscanf(fp,"%d",&shmid); 
      
    // Attach shared memory
    bbStruct_t *shmem;
    if ((shmem = (bbStruct_t *) shmat(shmid, NULL, SHM_RND)) == (void *)-1) {
        printf("Failed to attach shared memory.\n");
        exit(1);
    }
    
    // Print data from shared memory   
    printf("\nHERE IS OUR SHARED MEMORY:\n\n"); 
    // There are 12 of these structs
    int i;
    for (i=0; i<12; i++) {
        printf("%2d: %20s| %8d| %30s|\n", shmem->id, shmem->name, shmem->favNum, shmem->favFood);
        shmem++;
    }
    printf("\n");
    return 0;
}
