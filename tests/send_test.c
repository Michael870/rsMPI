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
    MPI_Request *reqs = NULL;
    int *flags = NULL;

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
        int count = 0;
        int index;

        sleep(1);
        reqs = (MPI_Request *)malloc((num_of_processes - 1) * sizeof(MPI_Request));
        flags = (int *)malloc((num_of_processes - 1) * sizeof(int));
        for(i=1;i<num_of_processes;i++) {
            MPI_Isend(sbuf, n_send, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &reqs[i-1]);
        }

        while(count < num_of_processes - 1){
            MPI_Waitany(num_of_processes - 1, reqs, &index, MPI_STATUS_IGNORE);
            count ++;
        }
        //MPI_Waitall(num_of_processes - 1, reqs, MPI_STATUSES_IGNORE);
       
//        for(i = 0; i < num_of_processes - 1; i++){
//            flags[i] = 0;
//            while(!flags[i]){
//                MPI_Test(&reqs[i], &flags[i], NULL);
//                if(!flags[i])
//                    sleep(1);
//            }
//        }
    } else {
        reqs = (MPI_Request *)malloc(sizeof(MPI_Request));
        flags = (int *)malloc(sizeof(int));
        MPI_Irecv(rbuf, n_send, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, reqs);
        MPI_Wait(reqs, status);
//        *flags = 0;
//        while(!(*flags)){
//            MPI_Test(reqs, flags, status);
//            if(!(*flags))
//                sleep(1);
//        }
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


    MPI_Finalize();
    return 0;
}
