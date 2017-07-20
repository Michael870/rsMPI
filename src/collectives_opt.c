#include <mpi.h>
//#include <mpi-ext.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include "mpi_override.h"
//#include "error_handler.h"
#include "shadow_coordinate.h"
#include "shared.h"
#include "request_list.h"
#include <math.h>



int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
    long long length;
    MPI_Aint lb, extent;

#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Bcast\n", ls_world_rank);
#endif
    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Bcast(buffer, count, datatype, root, ls_data_world_comm);
        }
        else{
            PMPI_Bcast(buffer, count, datatype, root, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(datatype, &lb, &extent);
        length = extent * (long long)count;
        socket_send(buffer, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, buffer);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Bcast, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Bcast\n", ls_world_rank);
#endif
#endif
    return MPI_SUCCESS; 
}


int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm){
    long long length;
    MPI_Aint lb, extent;
    int rc;
    int comm_rank, comm_size;

    MPI_Comm_rank(comm, &comm_rank);
    MPI_Comm_size(comm, &comm_size);
 
#ifdef DEBUG
    printf("[%d] begin Reduce\n", ls_world_rank);
#endif
    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, ls_data_world_comm);
        }
        else{
            rc = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
        }
        /*forward result to shadow*/
        if(comm_rank == root){
            MPI_Type_get_extent(datatype, &lb, &extent);
            length = extent * (long long)count;
            socket_send(recvbuf, length, 0, 0); 
        }
    }
    else if(comm_rank == root){
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
    printf("[%d] end Reduce\n", ls_world_rank);
#endif
 
    return rc;
}


int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
    long long length;
    MPI_Aint lb, extent;
    int rc;

#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Allreduce\n", ls_world_rank);
#endif

    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, ls_data_world_comm);
        }
        else{
            PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(datatype, &lb, &extent);
        length = extent * (long long)count;
        socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Allreduce, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Allreduce\n", ls_world_rank);
#endif
#endif
 
    return rc;
}


int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Alltoall\n", ls_world_rank);
#endif

    int rc;
    MPI_Aint lb, data_size;
    long long length;
    int size;

    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, ls_data_world_comm);
        }
        else{
            PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(recvtype, &lb, &data_size);
        MPI_Comm_size(comm, &size);
        length = data_size * (long long)recvcount * size;
        socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
    
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Alltoall, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Alltoall\n", ls_world_rank);
#endif
#endif
 
    return rc;
}


int MPI_Alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Alltoallv\n", ls_world_rank);
#endif

    int rc;
    MPI_Aint lb, data_size;
    int i, max_i, count;
    int comm_size;
    int length;
    double myt1, myt2;

    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, ls_data_world_comm);
        }
        else{
            PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(recvtype, &lb, &data_size);
        MPI_Comm_size(comm, &comm_size);
        max_i = comm_size - 1;
        /*assuming no overlap of buffer writing, find the max displacement*/
        for(i = 0; i < comm_size; i++){
            if(rdispls[i] > rdispls[max_i])
                max_i = i;
        }
        count = rdispls[max_i] + recvcounts[max_i];
        length = data_size * (long long)count;
        myt1 = MPI_Wtime();
        socket_send(recvbuf, length, 0, 0);
        myt2 = MPI_Wtime();
        //printf("[%d] msg length is %d bytes, took %.2f seconds\n", ls_world_rank, length, myt2 - myt1); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Alltoallv, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Alltoallv\n", ls_world_rank);
#endif
#endif
 
    return rc;
}

int MPI_Barrier(MPI_Comm comm){
    //int sbuf, rbuf;
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Barrier\n", ls_world_rank);
#endif

    int rt;
    
    if(comm == MPI_COMM_WORLD){
        PMPI_Barrier(ls_data_world_comm);
    }
    else{
        PMPI_Barrier(comm);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Barrier, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Barrier\n", ls_world_rank);
#endif
#endif
    return rt;
}

int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            int dest, int sendtag, void *recvbuf, int recvcount,
                MPI_Datatype recvtype, int source, int recvtag,
                    MPI_Comm comm, MPI_Status *status){
    int rt;
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Sendrecv, dest is %d, src is %d\n", ls_world_rank, dest, source);
#endif
        MPI_Request temp_reqs[2];
        
        rt = MPI_Isend(sendbuf, sendcount, sendtype, dest, sendtag, comm, &temp_reqs[0]);
        if(rt != MPI_SUCCESS){
            return rt;
        }
        rt = MPI_Irecv(recvbuf, recvcount, recvtype, source, recvtag, comm, &temp_reqs[1]);
    
        MPI_Wait(&temp_reqs[0], MPI_STATUS_IGNORE);
        MPI_Wait(&temp_reqs[1], status);
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Sendrecv, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Sendrecv\n", ls_world_rank);
#endif
#endif
    return rt;
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Gather\n", ls_world_rank);
#endif

    int rc = MPI_SUCCESS;//initialized to be success, so that if comm only has one process rc is automatically set to success
    long long length;
    MPI_Aint lb, extent;
    int comm_rank, comm_size;

    MPI_Comm_rank(comm, &comm_rank);
    MPI_Comm_size(comm, &comm_size);
    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, ls_data_world_comm);
        }
        else{
            PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
        }
        /*forward result to shadow*/
        if(comm_rank == root){
            MPI_Type_get_extent(recvtype, &lb, &extent);
            length = extent * (long long)recvcount * comm_size;
            socket_send(recvbuf, length, 0, 0); 
        }
    }
    else if(comm_rank == root){
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Gather, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Gather\n", ls_world_rank);
#endif
#endif
 
    return rc;
}

int MPI_Allgather(const void *sendbuf, int  sendcount,
             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm)
{
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Allgather\n", ls_world_rank);
#endif

    int rc = MPI_SUCCESS;//initialized to be success, so that if comm only has one process rc is automatically set to success
    long long length;
    MPI_Aint lb, extent;
    int comm_size;

    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, ls_data_world_comm);
        }
        else{
            PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
        }
        /*forward result to shadow*/
            MPI_Type_get_extent(recvtype, &lb, &extent);
            MPI_Comm_size(comm, &comm_size);
            length = extent * (long long)recvcount * comm_size;
            socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Allgather, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Allgather\n", ls_world_rank);
#endif
#endif
    return rc;
}
    
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype
            recvtype, int root, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Gatherv\n", ls_world_rank);
#endif
    int rc;
    MPI_Aint lb, data_size;
    int i, max_i, count;
    int comm_size, comm_rank;
    int length;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);
    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, ls_data_world_comm);
        }
        else{
            PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
        }
        /*forward result to shadow*/
        if(comm_rank == root){
            MPI_Type_get_extent(recvtype, &lb, &data_size);
            max_i = comm_size - 1;
            /*assuming no overlap of buffer writing, find the max displacement*/
            for(i = 0; i < comm_size; i++){
                if(displs[i] > displs[max_i])
                    max_i = i;
            }
            count = displs[max_i] + recvcounts[max_i];
            length = data_size * (long long)count;
            socket_send(recvbuf, length, 0, 0); 
        }
    }
    else if(comm_rank == root){
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Gatherv, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Gatherv\n", ls_world_rank);
#endif
#endif
 
    return rc;
}

int MPI_Allgatherv(const void *sendbuf, int sendcount,
            MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                const int displs[], MPI_Datatype recvtype, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Allgatherv\n", ls_world_rank);
#endif
    int rc;
    MPI_Aint lb, data_size;
    int i, max_i, count;
    int comm_size, comm_rank;
    int length;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);
    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, ls_data_world_comm);
        }
        else{
            PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(recvtype, &lb, &data_size);
        max_i = comm_size - 1;
        /*assuming no overlap of buffer writing, find the max displacement*/
        for(i = 0; i < comm_size; i++){
            if(displs[i] > displs[max_i])
                max_i = i;
        }
        count = displs[max_i] + recvcounts[max_i];
        length = data_size * (long long)count;
        socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Allgatherv, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Allgatherv\n", ls_world_rank);
#endif
#endif
 
    return rc;
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                               int root, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Scatter\n", ls_world_rank);
#endif
    int rc;
    MPI_Aint lb, data_size;
    int comm_size, comm_rank;
    int length;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);
    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, ls_data_world_comm);
        }
        else{
            PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(recvtype, &lb, &data_size);
        length = data_size * (long long)recvcount;
        socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Scatter, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Scatter\n", ls_world_rank);
#endif
#endif
 
    return rc;
}

int MPI_Scatterv(const void *sendbuf, const int sendcounts[], const int displs[],
                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, int root, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Scatterv\n", ls_world_rank);
#endif
    int rc;
    MPI_Aint lb, data_size;
    int comm_size, comm_rank;
    int length;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);
    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, ls_data_world_comm);
        }
        else{
            PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(recvtype, &lb, &data_size);
        length = data_size * (long long)recvcount;
        socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Scatterv, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Scatterv\n", ls_world_rank);
#endif
#endif
 
    return rc;
}

int MPI_Scan(const void *sendbuf, void *recvbuf, int count,
                            MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Scan\n", ls_world_rank);
#endif
    int rc;
    MPI_Aint lb, data_size;
    int length;

    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Scan(sendbuf, recvbuf, count, datatype, op, ls_data_world_comm);
        }
        else{
            PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(datatype, &lb, &data_size);
        length = data_size * (long long)count;
        socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Scan, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Scan\n", ls_world_rank);
#endif
#endif
 
    return rc;
}

int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Reduce_scatter\n", ls_world_rank);
#endif
    int rc;
    MPI_Aint lb, data_size;
    int length;
    int comm_rank;

    MPI_Comm_rank(comm, &comm_rank);
 
    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, ls_data_world_comm);
        }
        else{
            PMPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(datatype, &lb, &data_size);
        length = data_size * (long long)recvcounts[comm_rank];
        socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Reduce_scatter, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Reduce_scatter\n", ls_world_rank);
#endif
#endif
 
    return rc;
}


int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
#ifdef DEBUG
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Reduce_scatter_block\n", ls_world_rank);
#endif
    int rc;
    MPI_Aint lb, data_size;
    int length;

    if(ls_my_category == 0){
        if(comm == MPI_COMM_WORLD){
            PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, ls_data_world_comm);
        }
        else{
            PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm);
        }
        /*forward result to shadow*/
        MPI_Type_get_extent(datatype, &lb, &data_size);
        length = data_size * (long long)recvcount;
        socket_send(recvbuf, length, 0, 0); 
    }
    else{
        int src, tag, length;

        mq_pop(&src, &tag, &length, recvbuf);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Reduce_scatter_block, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Reduce_scatter_block\n", ls_world_rank);
#endif
#endif
 
    return rc;
}
