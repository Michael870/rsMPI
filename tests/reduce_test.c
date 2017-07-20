#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>

int do_something(int rank, int root) {
    return (2*rank) + root + 1;
}

int main(int argc,char *argv[])
{
    int rank, num_of_processes;
    int i;
    int root = 0;

    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc,&argv);
    MPI_Comm_size( comm, &num_of_processes);
    MPI_Comm_rank( comm, &rank);

    int localsum[3] = {0};
    int globalsum[3] = {0};
    int expectedsum[3] = {0};
    
    if(rank == 0) {
        printf("Checking mpi_reduce(sum)... (if you see no output then you are good)\n");
    }

    for(root=0; root<num_of_processes; root++) {
        localsum[0] = do_something(rank, root);
        localsum[1] = localsum[0] * 2 - 1;
        localsum[2] = localsum[0] / 2; 
        MPI_Reduce(&localsum,&globalsum,3,MPI_INT,MPI_SUM,root,MPI_COMM_WORLD);
        if(rank == root) {
            for(i=0; i<num_of_processes; i++) {
                expectedsum[0] = expectedsum[0] + do_something(i, root);
                expectedsum[1] = expectedsum[1] + do_something(i, root) * 2 - 1;
                expectedsum[2] = expectedsum[2] + do_something(i, root) / 2;
            }
            if (globalsum[0] != expectedsum[0]) {
                printf("ERROR: Expected %d got %d [root:%d]\n", expectedsum[0], globalsum[0], root);
            }
   
            if (globalsum[1] != expectedsum[1]) {
                printf("ERROR: Expected %d got %d [root:%d]\n", expectedsum[1], globalsum[1], root);
            }
    
            if (globalsum[2] != expectedsum[2]) {
                printf("ERROR: Expected %d got %d [root:%d]\n", expectedsum[2], globalsum[2], root);
            }
    
        }
    }
    sleep(5);
    MPI_Finalize();
}
