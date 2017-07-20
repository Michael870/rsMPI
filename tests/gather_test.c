#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int rank;
    int num_of_processes;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("Found size to be %d \n", num_of_processes);

    /* try and gather some random information from all the nodes and
       verify that it is what is expected.*/

    if(rank == 0) {
        printf("Checking mpi_gather... (if you see no output then you are good)\n");
    }

    /* every node will get the opportunity to be root so we the
       receive buffers on each node.
    */
    int *rbuf; 
    rbuf = (int *)malloc(num_of_processes*100*sizeof(int)); 

    int root = 0;
    for(root = 0; root<num_of_processes; root++) {

        int i = 0, j = 0;
        MPI_Comm comm = MPI_COMM_WORLD; 
        int sendarray[100];
    
    
        for(i=0; i<100; i++) {
            sendarray[i] = rank+i;
        }
        MPI_Gather( sendarray, 100, MPI_INT, rbuf, 100, MPI_INT, root, comm); 
        
        if (rank == root) {
            for(j=0; j<num_of_processes; j++) {
                for(i=0;i<100;i++) {
                    if(rbuf[(j*100)+i] != j+i) {
                        printf("ERROR: we expected %d got %d [root:%d]\n", (j+i), rbuf[(j*100)+i], root);
                    }
                }
            }
        }
    }
    sleep(5);
    MPI_Finalize();
    return 0;
}
