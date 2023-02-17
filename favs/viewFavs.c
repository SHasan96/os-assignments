// Name: Saif Hasan
// CSC 460
// Date: 02/17/23
// Favs List (View the list)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>

typedef struct {
    int id;
    char name[20];
    int favNum;
    char favFood[30];
} bbStruct_t;

int main() {
   // shmem id = 32797 from /pub/csc460/bb/BBID.txt
   
   // Attach shared memory
   bbStruct_t *shmem;
   shmem = (bbStruct_t *) shmat(32797, NULL, SHM_RND);
   
   printf("HERE IS OUR SHARED MEMORY:\n"); 
   // There are 12 of these structs
   int i;
   for (i=0; i<12; i++) {
       printf("%2d: %20s| %8d| %30s|\n", shmem->id, shmem->name, shmem->favNum, shmem->favFood);
       shmem++;
   }
   return 0;
}
