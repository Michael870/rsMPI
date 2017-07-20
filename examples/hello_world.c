#include "mpi.h"
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
//#include "mpi_override.h"

#define COMPUTE_I 30000
#define COMPUTE_J 30000

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
 
int main(int argc, char** argv){
    int size;
    int rank;
    int mat_size = 1000;
    int i = 0;
    char buf[64][64];
    int len[64];

    if(argc == 2){
        mat_size = atoi(argv[1]);
    }


    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   
    if(rank == 0){ 
        for(i = 0; i < 64; i++){
            if(i % 3 == 0){
                strcpy(buf[i], "hello");
            }
            else if(i % 3 == 1){
                strcpy(buf[i], "world");
            }
            else if(i %3 == 2){
                strcpy(buf[i], "what the hell");
            }
            len[i] = strlen(buf[i]) + 1;
        }
    }
    else{
        for(i = 0; i < 64; i++){
            strcpy(buf[i], "xxx");
        }
    }

    for(i = 0; i < 64; i++){
        if(rank == 0){
            MPI_Send(buf[i], len[i], MPI_CHAR, 1, i, MPI_COMM_WORLD);
            matrix_multiply(mat_size);
        }
        else if(rank == 1){
            MPI_Recv(buf[i], 64, MPI_CHAR, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Recv msg: %s\n", buf[i]);
            matrix_multiply(mat_size);
        } 
    }
    MPI_Finalize();
    
    return 0;
}
