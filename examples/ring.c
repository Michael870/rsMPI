// Author: Wes Kendall
// Copyright 2011 www.mpitutorial.com
// This code is provided freely with the tutorials on mpitutorial.com. Feel
// free to modify it for your own use. Any distribution of the code must
// either provide a link to www.mpitutorial.com or keep this header in tact.
//
// Example using MPI_Send and MPI_Recv to pass a message around in a ring.
//
#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define LOOP 30 
#define MAT_SIZE 800

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

void two_comm(int start, int end){
    int rank, size, sendbuf, recvbuf;
    int i;
    int prev, next;
    MPI_Status status;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    next = (rank + 1) % size;
    if(rank != 0)
        prev = rank - 1;
    else
        prev = size - 1;

    for(i = start; i < end; i++){
        MPI_Sendrecv(&sendbuf, 1, MPI_INT, next, 0, &recvbuf, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, &status);
        if(i % 3 == 2){
            MPI_Allreduce(&sendbuf, &recvbuf, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        } 
        if(rank == 2)
            printf("[%d] Skipped iteration %d\n", rank, i);
    }
}


int main(int argc, char** argv) 
{
  MPI_Init(NULL, NULL);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  int loop_index; 
  MPI_Status status;
  int sendbuf[16];
  int recvbuf[16];  
  int next = (world_rank + 1) % world_size;
  int prev;
  if(world_rank != 0)
     prev = (world_rank - 1) % world_size;
  else
     prev = world_size - 1;

  for(loop_index = 0; loop_index < LOOP; loop_index++){
//      MPI_Sendrecv(&sendbuf, 1, MPI_INT, next, 0, &recvbuf, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, &status);
      if(world_rank == 0){
          MPI_Send(sendbuf, 16, MPI_INT, next, 0, MPI_COMM_WORLD);
          MPI_Recv(recvbuf, 16, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      else{
          MPI_Recv(recvbuf, 16, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          MPI_Send(sendbuf, 16, MPI_INT, next, 0, MPI_COMM_WORLD);
      }

      matrix_multiply(MAT_SIZE);
      printf("[%d] Iteration %d\n", world_rank, loop_index);
  }
//  MPI_Barrier(MPI_COMM_WORLD);
  if(world_rank == 0)
      printf("All finished\n");

          
  MPI_Finalize();
}
