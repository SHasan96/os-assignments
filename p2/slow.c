// Name: Saif Hasan
// CSC 460
// Date: 2/10/2023
// Slow Synch - Fork and Files

#include <stdio.h>

main(int argc, char **argv) {
    FILE *fopen(), *fp;
    
    //Assuming provided argument is an integer
    int n;
    if (argc < 2) {
        printf("Please provide an integer argument within 1-26 (inclusive).\n");
        return 0;
    } else {
        n = atoi(argv[1]);
    }
    
    //Validate the argument
    if (n<1 || n>26) {
        printf("Invalid argument! The integer argument must be within 1-26 (inclusive).\n");
        return 0;
    }

    //Open File to write a value  
    if((fp = fopen("syncfile","w")) == NULL) {
        printf("Failed to open file for writing.\n");
        return 0;
    }

    //Write into syncfile the initial process' char id
    fprintf(fp,"%c", 'A');
    fclose(fp);

    //Create child processes and give unique myids
    char  myid = 'A';
    char  lastid = 'A' + (n-1);
    //printf("lastid = %c\n", lastid);  
    int j;
    for(j=1; j<n; j++) { // starting at j=1 
        if (fork() == 0) 
            myid++; // next child is assigned a myid which is the next letter
        else 
            break;
    }
          
    // Loop 5 times (with the relevant processes)
    int  i = 0;
    char  found;

    for (i=0; i<5; i++) {
        //Repeatedly check file until myid is found 
        found = ' ';
        while (found != myid) {
            if((fp = fopen("syncfile","r")) == NULL) {
                printf("Failed to open file for reading.\n");
                return 0;
            }
            fscanf(fp,"%c",&found);
            fclose(fp);
        }
     
        // It must be my turn to do something.....      
        printf("%c: %d\n", myid, getpid());

        // Update file to allow the next id to go
        if((fp = fopen("syncfile","w")) == NULL) {
            printf("Failed to open file for writing.\n");
            return 0;
        }

        if (myid == lastid)
            fprintf(fp,"%c", 'A'); //when last id is reached
        else
            fprintf(fp, "%c", myid+1); //write next id

        fclose(fp);
    } 
    wait(); // wait for child processes
            // this makes sure the prompt on the terminal does not interfere outputs
    return 0;
}
