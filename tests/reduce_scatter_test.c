#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>

int main(int argc,char *argv[])
{
    int rank, num_of_processes;
    int *rdata,*sdata,*rcounts,*expected_data;
    int i,j;

    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc,&argv);
    MPI_Comm_size( comm, &num_of_processes);
    MPI_Comm_rank( comm, &rank);
    sdata = malloc(sizeof(int)*num_of_processes); 
    rcounts = malloc(sizeof(int)*num_of_processes); 
    // malloc more than necessary .. its a test
    rdata = malloc(sizeof(int)*num_of_processes);
    expected_data = malloc(sizeof(int)*num_of_processes); 

    /**
     * Building an send arrays like the following.
     * rank_0 = 0 1 2
     * rank_1 = 1 2 3
     * rank_2 = 2 3 4
     *
     * After reduce the receive buffers should each get equal number of items
     * rank_0 = 3 
     * rank_1 = 6 
     * rank_2 = 9
     *
     * 
     **/

    for(i=0; i<num_of_processes; i++) {
        sdata[i] = rank+i;
        expected_data[i] = 0;
        rcounts[i] = 1;
    }

    if(rank == 0) {
        printf("Checking mpi_reduce_scatter... (if you see no output then you are good)\n");
    }

    // need to get expected values!
    for(i=0; i<num_of_processes; i++) {
        for(j=0; j<rcounts[i]; j++) {
            expected_data[j] += i+rank;
        }
    }

    MPI_Reduce_scatter( sdata, rdata, rcounts, MPI_INT, MPI_SUM, MPI_COMM_WORLD );

    for(j=0; j<rcounts[rank]; j++) {
        if(rdata[j] != expected_data[j]) {
            printf("ERROR: Received at rank %d rdata[%d] = %d expected %d\n", rank, j, rdata[j], expected_data[j]);
        } 
    }
    sleep(2);
    MPI_Finalize();
    return 0;
}

