#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>

int main(int argc, char **argv)
{
    int rank;
    int num_of_processes;
    int i;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("Rank : %d | Size %d\n", rank, num_of_processes);

    int *sbuf = malloc(2*sizeof(int));
    int *rbuf = malloc(2*sizeof(int));
    sbuf[0] = 111;
    sbuf[1] = 123;
    
    MPI_Request *request = malloc(sizeof(MPI_Request)*num_of_processes);
    MPI_Status *status = malloc(sizeof(MPI_Status));

    
    if(rank == 0) {
        for(i=1;i<num_of_processes;i++) {
            MPI_Isend(sbuf, 2, MPI_INT, i, 1, MPI_COMM_WORLD, &request[i-1]);
        }
        for(i=1;i<num_of_processes;i++) {
            MPI_Wait(&request[i-1], status);
        }
    } else {
        MPI_Recv(rbuf, 2, MPI_INT, 0, 1, MPI_COMM_WORLD, status);
    }


    //    MPI_Issend(sbuf, 2, MPI_INT, 0, 1, MPI_COMM_WORLD, request);
    //
    //    if(rank == 0) {
    //        for(i=1;i<num_of_processes;i++) {
    //            MPI_Recv(rbuf, 2, MPI_INT, i, 1, MPI_COMM_WORLD, status);
    //        }
    //    }
    //
    //    MPI_Wait(request, status);


    MPI_Finalize();
    return 0;
}
