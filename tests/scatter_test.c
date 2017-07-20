#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>

int main(int argc,char *argv[])
{
    int rank, num_of_processes;
    int *recvbuffer,*sendbuffer;
    int count;
    int i;
    int datasize;
    int root = 0;

    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc,&argv);
    MPI_Comm_size( comm, &num_of_processes);
    MPI_Comm_rank( comm, &rank);

    if(rank == 0) {
        printf("Checking mpi_scatter... (if you see no output then you are good)\n");
    }

    for(root=0; root<num_of_processes; root++) {

        /*each processor will get count elements(in recvbuffer) from the root(through MPI_Scatter)*/
        count=4;
        recvbuffer=(int*)malloc(count*sizeof(int));
        /*Root Process initializes sendbuffer*/
        if(rank== root){
            datasize=count * num_of_processes;
            sendbuffer=(int*)malloc(datasize*sizeof(int));
            for(i=0;i<datasize;i++){
                sendbuffer[i]=i;
                //            printf("Sendbuffer[%d] = %d\n",i,sendbuffer[i]);
            }
        }
        /*Distribute Data through MPI_Scatter*/
        MPI_Scatter(sendbuffer,count,MPI_INT,recvbuffer,count,MPI_INT,root,MPI_COMM_WORLD);
    
        for(i=0;i<count;i++) {
            if(recvbuffer[i] != (rank*count)+i) {
                printf("ERROR: expected %d got %d. [rank:%d]\n", (rank*count)+1, recvbuffer[i], rank);
            }
        }
    }
    sleep(5);
    MPI_Finalize();
    return 0;
}

