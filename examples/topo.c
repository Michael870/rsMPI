#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
    int rank, size;
    MPI_Comm newcomm;
    MPI_Comm acomm;
    int dims[3] = {4, 3, 4};
    int periods[3] = {1, 1, 0};
    int coords[3];
    int index[4] = {2, 3, 4, 6};
    int edges[6] = {1, 3, 0, 3, 0, 2};
    int rank_1, rank_2, size_1, size_2;
    int remain_dims[3] = {0, 1, 1};

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &acomm);
    MPI_Graph_create(MPI_COMM_WORLD, 4, index, edges, 0, &newcomm); 
    MPI_Topo_test(acomm, &rank_1);
    if(rank <= 3){
        int nnodes, nedges;
        int nneighbors;
        int neighbors[16];
        char str[64];
        char buffer[8];
        int i;

        MPI_Graphdims_get(newcomm, &nnodes, &nedges);
        //printf("Rank %d: %d %d\n", rank, nnodes, nedges);
        MPI_Graph_neighbors_count(newcomm, rank, &nneighbors);
        MPI_Graph_neighbors(newcomm, rank, 16, neighbors);
        if(nneighbors == 0){
            strcpy(str, "I don't have neighbor");
        }
        else{
            strcpy(str, "My neighbors are: ");
            for(i = 0; i < nneighbors; i++){
                sprintf(buffer, " %d", neighbors[i]);
                strcat(str, buffer);
            }
        }
        printf("Rank %d: %s\n", rank, str);
    }
    //MPI_Topo_test(newcomm, &rank_2);
    //if(rank_2 != MPI_GRAPH)
    //    printf("Rank %d: newcomm not GRAPH!\n", rank);

    MPI_Finalize();
    return 0;
}
