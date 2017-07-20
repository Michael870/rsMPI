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

    //    printf("Rank : %d | Size %d\n", rank, num_of_processes);

    int *sbuf = malloc(2*sizeof(int));
    int *rbuf = malloc(2*sizeof(int));
    int e0 = 111, e1 = 123;
    sbuf[0] = e0;
    sbuf[1] = e1;
    
    MPI_Request *request = malloc(sizeof(MPI_Request));
    MPI_Status *status = malloc(sizeof(MPI_Status));

    
    if(rank == 0) {
        for(i=0;i<num_of_processes;i++) {
            MPI_Send(sbuf, 2, MPI_INT, i, 1, MPI_COMM_WORLD);
        }
    } else {
        MPI_Irecv(rbuf, 2, MPI_INT, 0, 1, MPI_COMM_WORLD, request);
        MPI_Wait(request, status);
        if(rbuf[0] != e0) {
            printf("Receive buffer [0] expected to be %d got %d\n", e0, rbuf[0]);
        }
        if(rbuf[1] != e1) {
            printf("Receive buffer [1] expected to be %d got %d\n", e1, rbuf[1]);
        }
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
