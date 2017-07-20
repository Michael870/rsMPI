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

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //    printf("Found size to be %d\n", num_of_processes);

    int *sbuf, *rbuf;
    int scount=1, rcount = 1;
    int cnt = 0;


    sbuf = malloc(sizeof(int)*scount);

    for(i=0; i<scount; i++) {
        sbuf[i] = rank+i;
    }
    
    if (rank < (num_of_processes-1)) {
        MPI_Send( sbuf, scount, MPI_INT, (num_of_processes-1), 1, MPI_COMM_WORLD);
    } else {
        // I'm the lucky receiver
        MPI_Status *status = malloc(sizeof(MPI_Status));
        rbuf = malloc(sizeof(int)*rcount);
        printf("RECV: Not sure how to write a test for this but each recv order should match...\n");
        for(i=0;i<num_of_processes-1;i++) {
            MPI_Recv( rbuf, rcount, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, status);
            cnt++;
            printf("RECV: Recv order %d from [%d] at [%d]\n", cnt, status->MPI_SOURCE, rank);
            fflush(NULL);
        }
    }

    // try the same thing with irecv
    MPI_Barrier(MPI_COMM_WORLD);
    if ((rank == num_of_processes-1)) {
        printf(" **** checking irecv rank [%d]\n", rank);
        fflush(NULL);
    }

    cnt = 0;

    if (rank < (num_of_processes-1)) {
        MPI_Send( sbuf, scount, MPI_INT, (num_of_processes-1), 1, MPI_COMM_WORLD);
    } else {
        // I'm the lucky receiver
        rbuf = malloc(sizeof(int)*rcount);
        printf("IRECV: Not sure how to write a test for this but each recv order should match...\n");
        for(i=0;i<num_of_processes-1;i++) {
            MPI_Request *request = malloc(sizeof(MPI_Request));
            MPI_Status *status = malloc(sizeof(MPI_Status));
            MPI_Irecv( rbuf, rcount, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, request);
            MPI_Wait(request, status);
            if(status->MPI_SOURCE == MPI_ANY_SOURCE) {
                printf( "ERROR: status returned any source\n" );
            }
            printf("IRECV: Recv order %d from [%d] at [%d]\n", cnt++, status->MPI_SOURCE, rank);
            fflush(NULL);
        }
    }

    MPI_Finalize();
    return 0;
}
