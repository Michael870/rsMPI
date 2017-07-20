#include "mpi.h"
#include <stdio.h>
#define SIZE 16
#define UP    0
#define DOWN  1
#define LEFT  2
#define RIGHT 3

int main(int argc,char **argv) {
    int numtasks, rank, source, dest, outbuf, i, tag=1;
    int inbuf[4]={MPI_PROC_NULL,MPI_PROC_NULL,MPI_PROC_NULL,MPI_PROC_NULL,};
    int nbrs[4], dims[2]={4,4};
    int periods[2]={1,1}, reorder=0, coords[2];
            
    MPI_Request reqs[8];
    MPI_Status stats[8];
    MPI_Comm cartcomm;
    int test_rank;
    
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    
    if (numtasks == SIZE) {
        MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cartcomm);
        MPI_Comm_rank(cartcomm, &rank);
        MPI_Cart_coords(cartcomm, rank, 2, coords);
        MPI_Cart_rank(cartcomm, coords, &test_rank);
        if(rank != test_rank) {
            printf("ERROR - Ranks did not match should be %d got %d\n", rank, test_rank);
        }

        MPI_Cart_shift(cartcomm, 0, 1, &nbrs[UP], &nbrs[DOWN]);
        MPI_Cart_shift(cartcomm, 1, 1, &nbrs[LEFT], &nbrs[RIGHT]);
        
        printf("rank= %d coords= %d %d  neighbors(u,d,l,r)= %d %d %d %d\n",
               rank,coords[0],coords[1],nbrs[UP],nbrs[DOWN],nbrs[LEFT],
               nbrs[RIGHT]);
                
        outbuf = rank;
                
        for (i=0; i<4; i++) {
            dest = nbrs[i];
            source = nbrs[i];
            MPI_Isend(&outbuf, 1, MPI_INT, dest, tag, 
                      MPI_COMM_WORLD, &reqs[i]);
            MPI_Irecv(&inbuf[i], 1, MPI_INT, source, tag, 
                      MPI_COMM_WORLD, &reqs[i+4]);
        }
        
        MPI_Waitall(8, reqs, stats);
        
        printf("rank= %d                  inbuf(u,d,l,r)= %d %d %d %d\n",
               rank,inbuf[UP],inbuf[DOWN],inbuf[LEFT],inbuf[RIGHT]);  }
    else
        printf("Must specify %d processors. Terminating.\n",SIZE);

    int num_of_processes;
    MPI_Comm_size(cartcomm, &num_of_processes);
    int root = 2;

    double dlocalsum[2];
    double dglobalsum[2];
    double *dexpectedsum = malloc(sizeof(double)*2);

    dlocalsum[0] = rank + root + 0.25;
    dlocalsum[1] = rank + root + 0.99212;
    MPI_Allreduce(dlocalsum,dglobalsum,2,MPI_DOUBLE,MPI_SUM,cartcomm);
    
    // check on all nodes!
    dexpectedsum[0] = 0;
    dexpectedsum[1] = 0;
    for(i=0; i<num_of_processes; i++) {
        dexpectedsum[0] = dexpectedsum[0] + i + root + 0.25;
        dexpectedsum[1] = dexpectedsum[1] + i + root + 0.99212;
    }
    
    if (dglobalsum[0] != dexpectedsum[0]) {
        printf("ERROR: Expected %f got %f [root:%d]\n", dexpectedsum[0], dglobalsum[0], root);
    }

    if (!((dglobalsum[1] < dexpectedsum[1]+0.000001) && (dglobalsum[1] > dexpectedsum[1]-0.000001))) {
        printf("ERROR: Expected %f got %f [root:%d]\n", dexpectedsum[1], dglobalsum[1], root);
    }


    dlocalsum[0] = rank + root + 0.299;
    MPI_Allreduce(dlocalsum,dglobalsum,1,MPI_DOUBLE,MPI_SUM,cartcomm);
    
    // check on all nodes!
    dexpectedsum[0] = 0;
    dexpectedsum[1] = 0;
    for(i=0; i<num_of_processes; i++) {
        dexpectedsum[0] = dexpectedsum[0] + i + root + 0.299;
    }
    
    if (!((dglobalsum[0] < dexpectedsum[0]+0.000001) && (dglobalsum[0] > dexpectedsum[0]-0.000001))) {
        printf("ERROR: Expected %f got %f [root:%d]\n", dexpectedsum[0], dglobalsum[0], root);
    }
    
    MPI_Finalize();
}
