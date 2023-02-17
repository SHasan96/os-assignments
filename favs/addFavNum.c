// Name: Saif Hasan
// CSC 460
// Date: 02/17/23
// Favs List (Add/Change favNum)

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

int main(int argc, char **argv) {
   // shmem id = 32797 from /pub/csc460/bb/BBID.txt
   if (argc == 1) {
       printf("An integer argument must be provided!\n");
       exit(1);
   }
   
   // Attach shared memory
   bbStruct_t *shmem;
   shmem = (bbStruct_t *) shmat(32797, NULL, SHM_RND);
   
   // I am number 4 (index 3) on the list.
   shmem += 3;
   shmem->favNum = atoi(argv[1]); // ignoring type checks
   return 0;
}
