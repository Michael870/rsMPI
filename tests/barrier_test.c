#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>

int main(int argc, char **argv)
{
    int rank;
    int dest;
    int num_of_processes;
    int tag = 2;
    int source;
    char message[100];
    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank !=0){
		/* create message */
		sprintf(message, "Greetings from process %d!", rank);
		dest = 0;
		/* use strlen+1 so that '\0' get transmitted */
		MPI_Send(message, strlen(message)+1, MPI_CHAR,
                 dest, tag, MPI_COMM_WORLD);
		MPI_Barrier(MPI_COMM_WORLD);
	}
	else{
		printf("From process 0: Num processes: %d\n",num_of_processes);
		for (source = 1; source < num_of_processes; source++) {
			MPI_Recv(message, 100, MPI_CHAR, source, tag,
                     MPI_COMM_WORLD, &status);
			printf("%s\n",message);
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}

    // cause a failure case.
    if(rank == 1) {
        printf("This should hang forever, you will have to ctrl-c to stop...\n");
        fflush(NULL);
		MPI_Barrier(MPI_COMM_WORLD);
        printf("ERROR - you should not get here if you have more than one process\n");
    }

    MPI_Finalize();
    return 0;
}
