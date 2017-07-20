#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc,char *argv[]){
	int *sray,*rray;
	int *sdisp,*scounts,*rdisp,*rcounts,*sdata,*rdata;
	int i;
    int j;
    int offset;
    int expected_num = 0;

    int rank;
    int num_of_processes;
	MPI_Init(&argc,&argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int stride = 100;

    sdata=(int*)malloc(sizeof(int)*num_of_processes*stride);
    rdata=(int*)malloc(sizeof(int)*num_of_processes*stride);
	scounts=(int*)malloc(sizeof(int)*num_of_processes);
	rcounts=(int*)malloc(sizeof(int)*num_of_processes);
	sdisp=(int*)malloc(sizeof(int)*num_of_processes);
	rdisp=(int*)malloc(sizeof(int)*num_of_processes);

    if(rank == 0) {
        printf("Getting ready to test alltoallv, if you see no output then you are good...\n");
    }

    /* how all-to-all works 

       All nodes send distinct i blocks of data to all i nodes. The
       receiving node will receive the data found in his rank position
       from each sending node but placed in the receiving array at the
       location of the sending rank.

       example:
       node 1 sending: 1 4 6
       node 2 sending: 6 3 9
       node 3 sending: 5 7 8

       after completion
       node 1 receiving: 1 6 5
       node 2 receiving: 4 3 7
       node 3 receiving: 6 9 8

       Note that receiving is a column of the sending matrix for each the node's rank
    */

    /* This should produce send arrays that look like this:
       node 1: 0  1  2  3  4  ...
       node 2: 0  2  4  6  8  ...
       node 3: 0  3  6  12 15 ...
    */
    for(i=0; i<num_of_processes; i++) {
        for(j=0; j<10; j++) {
            sdata[i*stride+j] = (rank*i)+j;
        }
        scounts[i] = 10;
        sdisp[i] = stride*i;

        rcounts[i] = 10;
        rdisp[i] = (stride/2)*i;
    }

    /* send the data */
    MPI_Alltoallv(sdata,scounts,sdisp,MPI_INT,
                  rdata,rcounts,rdisp,MPI_INT,
                  MPI_COMM_WORLD);

    for(i=0;i<num_of_processes; i++) {
        offset = (stride/2)*i;
        for(j=0;j<10;j++) {
            expected_num = (i*rank)+j;
            if(rdata[offset+j] != expected_num) {
                printf("ERROR: At rank %d expected %d got %d from node %d\n", rank, expected_num, rdata[offset+j], i);
            }
        }
    }
    MPI_Finalize();
}

