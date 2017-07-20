#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc,char *argv[]){
	int *sdisp,*scounts,*rdisp,*rcounts;
	int i, j;
    int pnum = 3;
    int *exp;

    int rank;
    int num_of_processes;
	MPI_Init(&argc,&argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	scounts=(int*)malloc(pnum * sizeof(int)*num_of_processes);
	rcounts=(int*)malloc(pnum * sizeof(int)*num_of_processes);
    exp = (int *)malloc(pnum * sizeof(int));

    if(rank == 0) {
        printf("Getting ready to test alltoall, if you see no output then you are good...\n");
    }

    for(i=0; i<pnum * num_of_processes; i++) {
        scounts[i] = rank;
    }

    /* send the data */
    MPI_Alltoall(scounts,pnum,MPI_INT,
                 rcounts,pnum,MPI_INT,
                 MPI_COMM_WORLD);

    for(i=0;i<num_of_processes; i++) {
        exp[0] = i;
        for(j = 0; j < pnum; j++){
            if(rcounts[i*pnum+ j] != exp[0]) {
                printf("ERROR: At rank %d expected %d got %d from node %d\n", rank, exp[0], rcounts[i*pnum], i);
            }
        }
    }
    sleep(5);
    MPI_Finalize();
}

