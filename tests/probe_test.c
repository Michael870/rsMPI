#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>

int main(int argc, char **argv)
{
    int rank;
    int num_of_processes;
    int i;
    int length;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("Rank : %d | Size %d\n", rank, num_of_processes);

    int n_send = 200;
    double *sbuf = malloc(n_send*sizeof(double));
    double *rbuf = malloc(n_send*sizeof(double));
    for(i=0;i<n_send;i++) {
        sbuf[i] = i*1.23;
    }
    
    MPI_Status *status = malloc(sizeof(MPI_Status));

    
    if(rank == 0) {
        sleep(2);
        for(i=1;i<num_of_processes;i++) {
            MPI_Send(sbuf, n_send, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
        }
    } else {
        int flag = 0;
        do{
            MPI_Iprobe(0, 1, MPI_COMM_WORLD, &flag, status);
            if(!flag){
                printf("Rank %d: no msg\n", rank);
                sleep(1);
            }
        }while(!flag);
        MPI_Get_count(status, MPI_DOUBLE, &length);
        printf("Rank %d: detected a msg with src = %d, tag = %d, len = %d\n", rank, status->MPI_SOURCE, status->MPI_TAG, length);
        MPI_Recv(rbuf, n_send, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, status);

        double e;
        for(i=0;i<n_send;i++) {
            e = i*1.23;
            if(rbuf[i] != e) {
                printf("Expected %f in receive buffer [%d] got %f\n", e, i, rbuf[i]);
            }
        }

        
    }

    sleep(15);

    //    MPI_Issend(sbuf, 2, MPI_INT, 0, 1, MPI_COMM_WORLD, request);
    //
    //    if(rank == 0) {
    //        for(i=1;i<num_of_processes;i++) {
    //            MPI_Recv(rbuf, 2, MPI_INT, i, 1, MPI_COMM_WORLD, status);
    //        }
    //    }
    //
    //    MPI_Wait(request, status);

    free(status);
    MPI_Finalize();
    return 0;
}
