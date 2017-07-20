#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>

int main(int argc,char *argv[])
{
    int rank, num_of_processes;
    int *recvbuffer,*sendbuffer;
    int *counts;
    int count;
    int *displs;
    int stride = 100;
    int i;
    int datasize;
    int root = 0;

    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc,&argv);
    MPI_Comm_size( comm, &num_of_processes);
    MPI_Comm_rank( comm, &rank);

    if(rank == 0) {
        printf("Checking mpi_scatterv... (if you see no output then you are good)\n");
    }
    
    count = 10;
    counts=(int*)malloc(num_of_processes*sizeof(int));
    displs=(int*)malloc(num_of_processes*sizeof(int));
    for(i=0; i<num_of_processes; i++) {
        counts[i] = count;
        displs[i] = i*stride;
    }               
    // stride is the amount "room"


    for(root=0; root<num_of_processes; root++) {
        
        /*Root Process initializes sendbuffer*/
        if(rank == root){
            datasize   = num_of_processes * stride;
            sendbuffer = (int*)malloc(num_of_processes*stride*sizeof(int));
            for(i=0;i<datasize;i++){
                sendbuffer[i]=i;
                //printf("Sendbuffer[%d] = %d\n",i,sendbuffer[i]);
            }
        }

        // should only be receiving count number, even though the
        // receive buffer has stride numbers
        recvbuffer=(int*)malloc(count*sizeof(int));
        /*Distribute Data through MPI_Scatter*/
        MPI_Scatterv(sendbuffer,counts,displs,MPI_INT,recvbuffer,count,MPI_INT,root,MPI_COMM_WORLD);
    
        for(i=0;i<count;i++) {
            //            printf("Receive buffer at rank %d root: %d : [%d]\n", rank, root, recvbuffer[i]);
            if(recvbuffer[i] != (rank*stride)+i) {
                printf("ERROR: expected %d got %d. [rank:%d]\n", (rank*stride)+i, recvbuffer[i], rank);
            }
        }
    }
    sleep(3);
    MPI_Finalize();
    return 0;
}
