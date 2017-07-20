#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

void compare(MPI_Comm comm){
    if(comm == MPI_COMM_WORLD)
        printf("Yes!\n");
    else
        printf("No!\n");
}

int main(int argc, char** argv){
    int size, rank;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm newcomm;
    MPI_Comm_dup(MPI_COMM_WORLD, &newcomm);
    printf("New communicator created!\n");
    compare(MPI_COMM_WORLD);
    compare(newcomm);
    MPI_Finalize();
    return 0;
}
