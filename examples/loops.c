#include "mpi.h"
#include <mpi-ext.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#define COMPUTE_I 30000
#define COMPUTE_J 3330
#define LOOP 20
#define SIZE 300
#define LEAPING 1

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

void communicate_func(int start, int end){
    int rank, size;
    int *send_buf = NULL; 
    int *recv_buf = NULL;
    int i;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    send_buf = (int*)malloc(SIZE * size * sizeof(int));
    recv_buf = (int*)malloc(SIZE * size * sizeof(int));
 
    for(i = start; i <  end; i++){   
        //MPI_Bcast(send_buf, SIZE, MPI_INT, 0, MPI_COMM_WORLD);
        int index;
        for(index = 0; index < size; index++){
            if(index != rank){
                MPI_Send(send_buf, 1, MPI_INT, index, 0, MPI_COMM_WORLD);
                MPI_Recv(recv_buf, 1, MPI_INT, index, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }    
        //MPI_Allreduce(send_buf, recv_buf, SIZE, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        printf("Rank %d: skipped iteration %d\n", rank, i); 
    }
    free(send_buf);
    free(recv_buf);
}
    
int main(int argc, char** argv){
    int size;
    int rank;
    int i, j;
    int buf;
    int sum = 0;
    struct timeval time_1, time_2, time_3, time_4;
    int num[SIZE*SIZE];

    int *send_buf = NULL; 
    int *recv_buf = NULL;
    if(argc != 3){
        printf("Arguments wrong (argc = %d)!\n", argc);
    }
//    int send_count = strtol(argv[1], NULL, 10);

    gettimeofday(&time_1, 0x0);
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#ifdef LEAPING
    //printf("Register state and function!\n");
    leap_register_state(&i, 1, MPI_INT);
    leap_register_state(num, SIZE*SIZE, MPI_INT);
//    leap_register_allreduce(send_count, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
//    leap_register_func( communicate_func);
#endif
    num[0] = rank;
    num[1] = 2 * rank;
    send_buf = (int*)malloc(SIZE * size * sizeof(int));
    recv_buf = (int*)malloc(SIZE * size * sizeof(int));
    gettimeofday(&time_3, 0x0);
    for(i = 0; i < LOOP; i++){
#ifdef LEAPING
//        if(i > 0 && i % 10 == 0)
//            trigger_leaping();
        ls_reset_recv_counter();
//        shadow_leap();
//        if(i % 5 == 4){
//            trigger_leaping();
//        }
#endif
        num[0] += i;
        num[1] += i;
        matrix_multiply(SIZE);
        int index;
        ls_inject_failure(atoi(argv[1]), atoi(argv[2]));
        for(index = 0; index < size; index++){
            if(index != rank){
                MPI_Send(send_buf, 1, MPI_INT, index, 0, MPI_COMM_WORLD);
                //if(index == size - 2)
                //    ls_inject_failure(atoi(argv[1]), atoi(argv[2]));
                MPI_Recv(recv_buf, 1, MPI_INT, index, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        matrix_multiply(SIZE);

        //MPI_Bcast(send_buf, SIZE, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Allreduce(send_buf, recv_buf, SIZE, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
//        if(rank == 0)
            printf("Rank %d: Iteration %d, num[0] = %d, num[1] = %d\n", rank, i, num[0], num[1]);
    }
    if(rank == 0){
        gettimeofday(&time_4, 0x0);
        double sec = (time_4.tv_sec - time_3.tv_sec);
        double usec = (time_4.tv_usec - time_3.tv_usec) / 1000000.0;
        double diff = sec + usec;
        printf("Computation time is %.3f seconds\n", diff); 
    }
    MPI_Finalize();
    
    gettimeofday(&time_2, 0x0);
    if(rank == 0){
        double sec = (time_2.tv_sec - time_1.tv_sec);
        double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
        double diff = sec + usec;
        printf("Total time is %.3f seconds\n", diff); 
    }
    free(send_buf);
    free(recv_buf);
    return 0;
}
