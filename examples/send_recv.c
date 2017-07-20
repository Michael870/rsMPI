#include <mpi.h>
//#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>

#define COMPUTE_I 60
#define COMPUTE_J 6000

//void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...); 

void my_compute(){
    int i, j;
    int a = 0, b = 3;
    double sum = 0;
    for(i = 0; i < COMPUTE_I; i++){
        for(j = 0; j < COMPUTE_J; j++){
            sum += log(a + 1) + exp(b);
            a++;
            b++;
            sum /= a + 2*b;
            sum *= 1.3777777;
            sum /= 1.361111;
        }
    }
}

int main(int argc, char** argv){
    int rank, size;
    MPI_Status status;

    int num = 40;
    MPI_Init(&argc, &argv);
    MPI_Comm cntr_comm;

    MPI_Comm_dup(MPI_COMM_WORLD, &cntr_comm);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request request_0, request_1;
    int sbuf[100] = {0};
    int rbuf[100] = {0};

//    my_compute();
    if(rank == 0){
        int buf1, buf2;
        MPI_Status status;    
        MPI_Sendrecv(&buf1, 1, MPI_INT, 1, 0, &buf2, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); 
    }
    else{
        int buf1, buf2;
        int count;
        MPI_Status status;    
        MPI_Sendrecv(&buf1, 1, MPI_INT, 0, 0, &buf2, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); 
        MPI_Get_count(&status, MPI_INT, &count);
        printf("Recved msg length = %d\n", count);
    } 
//    if(rank == 0){
//        MPI_Isend(&sbuf, num, MPI_INT, 1, 0, MPI_COMM_WORLD, &request_0);
//        MPI_Irecv(&rbuf, num, MPI_INT, 1, 0, MPI_COMM_WORLD, &request_1);
//        MPI_Wait(&request_0, MPI_STATUS_IGNORE);
//        MPI_Wait(&request_1, MPI_STATUS_IGNORE);
//    }
//    else if(rank == 1){
//        MPI_Isend(&sbuf, num, MPI_INT, 0, 0, MPI_COMM_WORLD, &request_0);
//        MPI_Irecv(&rbuf, num, MPI_INT, 0, 0, MPI_COMM_WORLD, &request_1);
//        MPI_Wait(&request_0, MPI_STATUS_IGNORE);
//        MPI_Wait(&request_1, MPI_STATUS_IGNORE);
//    }
    printf("rank %d, done!\n", rank);

    MPI_Finalize();
    return 0;
}

//void verbose_errhandler(MPI_Comm* pcomm, int* perr, ...) {
//    MPI_Comm comm = *pcomm;
//    int err = *perr;
//    char errstr[MPI_MAX_ERROR_STRING];
//    int i, rank, size, nf, len, eclass;
//    MPI_Group group_c, group_f;
//    int *ranks_gc, *ranks_gf;
//
//    MPI_Error_class(err, &eclass);
//    if( MPIX_ERR_PROC_FAILED != eclass ) {
//        MPI_Abort(comm, err);
//    }
//
//    MPI_Comm_rank(comm, &rank);
//    MPI_Comm_size(comm, &size);
//
//    MPIX_Comm_failure_ack(comm);
//    MPIX_Comm_failure_get_acked(comm, &group_f);
//    MPI_Group_size(group_f, &nf);
//    MPI_Error_string(err, errstr, &len);
//    printf("Rank %d / %d: Notified of error %s. %d found dead: { ",
//           rank, size, errstr, nf);
//
//    ranks_gf = (int*)malloc(nf * sizeof(int));
//    ranks_gc = (int*)malloc(nf * sizeof(int));
//    MPI_Comm_group(comm, &group_c);
//    for(i = 0; i < nf; i++)
//        ranks_gf[i] = i;
//    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
//                              group_c, ranks_gc);
//    for(i = 0; i < nf; i++)
//        printf("%d ", ranks_gc[i]);
//    printf("}\n");
//}
