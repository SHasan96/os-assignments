// Name: Saif Hasan
// CSC 460
// Date: 02/17/23
// Favs List (Update name)

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
   // shmem id = 32797 from /pub/csc460/bb/BBID.txt
   if (argc == 1) {
       printf("No argument provided!\n"); 
       exit(1);       
   } 
   // Attach shared memory
   bbStruct_t *shmem;
   shmem = (bbStruct_t *) shmat(32797, NULL, SHM_RND);
 
   // I am number 4 (index 3) on the list.
   shmem += 3;
   strncpy(shmem->name, argv[1], 19); 
   return 0;
}
