#include <stdio.h>
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int rank, grank;
    int size, gsize;
    MPI_Comm newcomm1, newcomm2;
    int dims[2] = {4, 4};
    int periods[2] = {1, 1};
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_dup(MPI_COMM_WORLD, &newcomm2);
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &newcomm1);
//    MPI_Graph_create(newcomm2, 4, index, edges, 0, &newcomm1);
    
    if(newcomm1 != MPI_COMM_NULL){
        int coords[2];
        int src, dest;
        
        MPI_Comm_rank(newcomm1, &rank);
        MPI_Cart_coords(newcomm1, rank, 2, coords);
        MPI_Cart_shift(newcomm1, 0, coords[1], &src, &dest);
        printf("%d/%d -> (%d, %d), my neighbors are %d and %d\n", rank, size, coords[0], coords[1], src, dest);
        MPI_Comm_free(&newcomm1);
    }
    MPI_Comm_free(&newcomm2);
    MPI_Finalize();
    sleep(3);
    return 0;
}
