#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "mpi.h"

#define BITS 3
int matrix_multiply(int size){
    int a[size][size];
    int b[size][size];
    int c[size][size];
    int i, j, k;

    srand(time(NULL));
    for(i = 0; i < size; i++)
        for(j = 0; j < size; j++){
            a[i][j] = rand() % 100;
            b[i][j] = rand() % 100;
        }
    for(i = 0; i < size; i++){
        for(j = 0; j < size; j++){
            int temp = 0;
            for(k = 0; k < size; k++){
                temp += a[i][k]*b[k][j];
            }
            c[i][j] = temp;
        }
    }
    return c[0][0];
}	

int main(){
    void *p = NULL;
    double *q;
    int *pp;
    int rank, size;
    int buf;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank < 2)
    {
        double time1, time2;
        time1 = MPI_Wtime();

        matrix_multiply(1000);
        time2 = MPI_Wtime();
        printf("%d: time %.3f\n", rank, time2 - time1);
    }
    else{
        MPI_Recv(&buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    MPI_Finalize();
//    p = malloc(1 << BITS);
//    if(p){
//        printf("Sucessfully malloc 2^%d bytes\n", BITS);
//    }
//    else{
//        printf("Failed to malloc 2^%d bytes\n", BITS);
//    }
//    free(p);
    return 0;
    
}
