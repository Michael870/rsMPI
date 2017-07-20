#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <math.h>
#include "mpi_override.h"
#include "shadow_coordinate.h"
#include "shadow_leap.h"
#include "shared.h"
#include "request_list.h"
#include "monitor_thread.h"
//#include "topo_list.h"

/*global variables*/
int ls_world_size, ls_world_rank, ls_app_size, ls_app_rank;
MPI_Comm ls_data_world_comm, ls_cntr_world_comm;
//MPI_Group ls_data_world_group, ls_main_group, ls_shadow_group;
int ls_my_category;// 0: main, 1: shadow, 2: shadow coordinator, 3: new main
int num_of_sc, shadow_ratio;
int ls_data_msg_count, ls_cntr_msg_count;
int ls_recv_counter; //# of recvs in current loop
int ls_leap_recv_counter; //# of recvs before leaping in current loop
int ls_my_coordinator;
int leap_flag;// 1: leap from main to shadow; 0: not to leap
//pthread_mutex_t mt_mutex;


int MPI_Init(int* argc, char*** argv){
    int ret;

#ifdef DEBUG
    printf("[%d] begin Init\n", ls_world_rank);
#endif
 
    ret = PMPI_Init(argc, argv);
    PMPI_Comm_size(MPI_COMM_WORLD, &ls_world_size);
    PMPI_Comm_rank(MPI_COMM_WORLD, &ls_world_rank);

    ls_init(); 
     
#ifdef DEBUG
    printf("[%d] end Init\n", ls_world_rank);
#endif
 
    return ret;
}



int MPI_Finalize(){
    int ret;
    int code;

#ifdef DEBUG
    printf("[%d]: begin Finalize.\n", ls_world_rank);
    fflush(stdout);
#endif

    if(0 == ls_my_category){
        code = -1;
        /*notify shadow coordinator of my completion*/
        PMPI_Send(&code, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
#ifdef DEBUG
        printf("[%d]: notified shadow coordinator that I am done.\n", ls_world_rank);
        fflush(stdout);
#endif
//        pthread_mutex_lock(&mt_mutex); //acquire mutex to ensure that helper thread is not holding the mutex
        pthread_kill(ls_monitor_thread, SIGUSR1);
        pthread_join(ls_monitor_thread, NULL);
//        pthread_mutex_unlock(&mt_mutex);
        free_thread_resources();
    }
    else if(1 == ls_my_category){
        /*disable signal from Shadow coordinator, to avoid calling MPI_FINALIZE twice*/
//        ls_sigaction_term.sa_handler = SIG_IGN;
//        sigaction(SIGUSR1, &ls_sigaction_term, NULL);
//        sigaction(SIGUSR2, &ls_sigaction_term, NULL);
//        sigaction(SIGALRM, &ls_sigaction_term, NULL);
        /* wait for main to finish
         * PMPI_Finalize() should be called in signal handler
         */
        while(1){
            ;
        }
    }
    else if(2 == ls_my_category){
        ; 
    }
    else{

#ifdef DEBUG
        printf("[%d]: ls_my_category undefined!\n", ls_world_rank);
        fflush(stdout);
#endif
    }

#ifdef DEBUG
    printf("[%d]: I am calling PMPI_Finalize().\n", ls_world_rank);
    fflush(stdout);
#endif
    free_resources();
    ret = PMPI_Finalize();
#ifdef DEBUG
    printf("[%d] end Finalize\n", ls_world_rank);
#endif
 
    return ret;
}
    
//int MPI_Comm_size(MPI_Comm comm, int *size){
//    int temp_size; 
//#ifdef DEBUG
//    printf("[%d] begin Comm_size\n", ls_world_rank);
//#endif
//    if(comm == MPI_COMM_WORLD){
//        PMPI_Comm_size(ls_data_world_comm, &temp_size);
//    }
//    else{
//        PMPI_Comm_size(comm, &temp_size);     
//    }
//    *size = temp_size / 2;
//#ifdef DEBUG
//    printf("[%d] end Comm_size, size = %d\n", ls_world_rank, *size);
//#endif
// 
//    return MPI_SUCCESS;
//}
//
//int MPI_Comm_rank(MPI_Comm comm, int *rank){
//#ifdef DEBUG
//    printf("[%d] begin Comm_rank\n", ls_world_rank);
//#endif
//    if(comm == MPI_COMM_WORLD){ 
//        *rank = ls_app_rank;
//    }
//    else{
//        int comm_size;
//        int comm_rank;
//        MPI_Comm_size(comm, &comm_size);
//        PMPI_Comm_rank(comm, &comm_rank);
//        *rank = comm_rank % comm_size;
//    }
//#ifdef DEBUG
//    printf("[%d] end Comm_rank, rank = %d\n", ls_world_rank, *rank);
//#endif
// 
//    return MPI_SUCCESS;
//}

int MPI_Comm_size(MPI_Comm comm, int *size){
#ifdef DEBUG
    printf("[%d] begin Comm_size\n", ls_world_rank);
#endif

    if(comm == MPI_COMM_WORLD){
        PMPI_Comm_size(ls_data_world_comm, size);
    }
    else{
        PMPI_Comm_size(comm, size);     
    }

#ifdef DEBUG
    printf("[%d] end Comm_size, size = %d\n", ls_world_rank, *size);
#endif
 
    return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank){
#ifdef DEBUG
    printf("[%d] begin Comm_rank\n", ls_world_rank);
#endif
    if(comm == MPI_COMM_WORLD){ 
        *rank = ls_app_rank;
    }
    else{
        PMPI_Comm_rank(comm, rank);
    }
#ifdef DEBUG
    printf("[%d] end Comm_rank, rank = %d\n", ls_world_rank, *rank);
#endif
 
    return MPI_SUCCESS;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){
//#ifdef DEBUG
    long long length;
    MPI_Aint lb, extent;

//    printf("[%d] begin MPI_Send\n", ls_world_rank);
//    fflush(stdout);
//#endif
    if(ls_my_category == 0){
        PMPI_Send(buf, count, datatype, dest, tag, comm);
#ifdef DEBUG
        MPI_Type_get_extent(datatype, &lb, &extent);
        length = extent * (long long)count;
        printf("[%d] end MPI_Send, dest=%d, tag=%d, length=%lld\n", ls_world_rank, dest, tag, length);
        fflush(stdout);
#endif
    }
#ifdef DEBUG
    else{
        printf("[%d] end MPI_Send\n", ls_world_rank);
        fflush(stdout);
    }
#endif
    
    return 0;
} 

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source,
                            int tag, MPI_Comm comm, MPI_Status *status){
    int length;
    MPI_Status temp_status; 

#ifdef DEBUG
    printf("[%d] begin MPI_Recv\n", ls_world_rank);
    fflush(stdout);
#endif

    if(ls_my_category == 0){
//        pthread_mutex_lock(&mt_mutex);
        if(leap_flag){
            PMPI_Send(&ls_recv_counter, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_RECV_POINT_TAG, ls_cntr_world_comm);
#ifdef DEBUG
            printf("[%d] Ready to leap. Notified shadow of my recv count = %d\n", ls_world_rank, ls_recv_counter);
            fflush(stdout);
#endif
 
//            if(leap_point_sent){
//#ifdef DEBUG
//                printf("[%d] Detected failure of another main at recv count = %d\n", ls_world_rank, ls_recv_counter);
//                fflush(stdout);
//#endif
//            }
//            else{
//#ifdef DEBUG
//                printf("[%d] Detected buffer overflow warining of my shadow at recv count = %d\n", ls_world_rank, ls_recv_counter);
//                fflush(stdout);
//#endif
//                PMPI_Send(&ls_recv_counter, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_RECV_POINT_TAG, ls_cntr_world_comm);
//                leap_point_sent = 1;
//#ifdef DEBUG
//                printf("[%d] Notified shadow of my recv count = %d\n", ls_world_rank, ls_recv_counter);
//                fflush(stdout);
//#endif
//            } 
            shadow_leap(1);
        }
//        pthread_mutex_unlock(&mt_mutex);
        if(status == MPI_STATUS_IGNORE){
            status = &temp_status;
        }

        PMPI_Recv(buf, count, datatype, source, tag, comm, status);
        PMPI_Get_count(status, MPI_CHAR, &length);

        socket_send(buf, length, status->MPI_SOURCE, status->MPI_TAG);
        ls_recv_counter++;
#ifdef DEBUG
        printf("[%d] end MPI_Recv, src=%d, tag=%d, length=%d, recv count = %d\n", ls_world_rank, status->MPI_SOURCE, status->MPI_TAG, length, ls_recv_counter);
        fflush(stdout);
#endif
    }
    else{
        int temp_src;
        int temp_tag;

        if(leap_flag && ls_recv_counter == ls_leap_recv_counter){
#ifdef DEBUG
            printf("[%d] Started to leap at recv count = %d\n", ls_world_rank, ls_recv_counter);
            fflush(stdout);
#endif
            shadow_leap(1);
        }
        mq_pop(&temp_src, &temp_tag, &length, buf);
        if(status != MPI_STATUS_IGNORE){
            status->MPI_SOURCE = temp_src;
            status->MPI_TAG = temp_tag;
            status->_ucount = length;
        }

        ls_recv_counter++;
#ifdef DEBUG
        printf("[%d] end MPI_Recv, src=%d, tag=%d, length=%d, recv count = %d\n", ls_world_rank, temp_src, temp_tag, length, ls_recv_counter);
        fflush(stdout);
#endif
    }


    return 0;
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status){
    int rc;
    int buf[3];

#ifdef DEBUG
    printf("[%d] begin MPI_Probe()\n", ls_world_rank);
    fflush(stdout);
#endif
    if(ls_my_category == 0){
        rc = PMPI_Probe(source, tag, comm, status);
        if(status != MPI_STATUS_IGNORE){
            buf[0] = status->MPI_SOURCE;
            buf[1] = status->MPI_TAG;
            buf[2] = status->_ucount;
        }
        PMPI_Send(buf, 3, MPI_INT, ls_world_rank + ls_app_size, SHADOW_PROBE_TAG, ls_cntr_world_comm);
        ls_cntr_msg_count++;
    }
    else{
        rc = PMPI_Recv(buf, 3, MPI_INT, ls_app_rank, SHADOW_PROBE_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        ls_cntr_msg_count++;
        if(status != MPI_STATUS_IGNORE){
            status->MPI_SOURCE = buf[0];
            status->MPI_TAG = buf[1];
            status->_ucount = buf[2];
        }
    }

#ifdef DEBUG
    printf("[%d] end MPI_Probe\n", ls_world_rank);
    fflush(stdout);
#endif

    return rc;
}


int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status){
    int rc;
    int buf[4];

#ifdef DEBUG
    printf("[%d] begin MPI_Iprobe()\n", ls_world_rank);
    fflush(stdout);
#endif
    if(ls_my_category == 0){
        rc = PMPI_Iprobe(source, tag, comm, flag, status);
        buf[0] = *flag;
        if(*flag && status != MPI_STATUS_IGNORE){
            buf[1] = status->MPI_SOURCE;
            buf[2] = status->MPI_TAG;
            buf[3] = status->_ucount;
        }
        PMPI_Send(buf, 4, MPI_INT, ls_world_rank + ls_app_size, SHADOW_PROBE_TAG, ls_cntr_world_comm);
        ls_cntr_msg_count++;
    }
    else{
        rc = PMPI_Recv(buf, 4, MPI_INT, ls_app_rank, SHADOW_PROBE_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        ls_cntr_msg_count++;
        *flag = buf[0];
        if(*flag && status != MPI_STATUS_IGNORE){
            status->MPI_SOURCE = buf[1];
            status->MPI_TAG = buf[2];
            status->_ucount = buf[3];
        }
    }

#ifdef DEBUG
    printf("[%d] end MPI_Iprobe()\n", ls_world_rank);
    fflush(stdout);
#endif
 
    return rc;
}


int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request){
#ifdef DEBUG
    long long length;
    MPI_Aint lb, extent;
    MPI_Type_get_extent(datatype, &lb, &extent);
    length = extent * (long long)count;
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
    printf("[%d] begin Isend, message length is %lld\n", ls_world_rank, length);
#endif
 
    if(ls_my_category == 0){ //I am a main process and need to duplicate msgs
        PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Isend, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Isend\n", ls_world_rank);
#endif
#endif

    return MPI_SUCCESS; 
}    

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request){
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
 
    int rc = 0;
#ifdef DEBUG
    printf("[%d] begin Irecv\n", ls_world_rank);
#endif

    if(ls_my_category == 0){
//        pthread_mutex_lock(&mt_mutex);
        if(leap_flag){
            PMPI_Send(&ls_recv_counter, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_RECV_POINT_TAG, ls_cntr_world_comm);
//#ifdef DEBUG
            printf("[%d] Ready to leap. Notified shadow of my recv count = %d\n", ls_world_rank, ls_recv_counter);
            fflush(stdout);
//#endif
 
    
//            if(leap_point_sent){
////#ifdef DEBUG
//                printf("[%d] Detected failure of another main at recv count = %d\n", ls_world_rank, ls_recv_counter);
//                fflush(stdout);
////#endif
//            }
//            else{
////#ifdef DEBUG
//                printf("[%d] Detected buffer overflow warining of my shadow at recv count = %d\n", ls_world_rank, ls_recv_counter);
//                fflush(stdout);
////#endif
//                PMPI_Send(&ls_recv_counter, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_RECV_POINT_TAG, ls_cntr_world_comm);
//                leap_point_sent = 1;
////#ifdef DEBUG
//                printf("[%d] Notified shadow of my recv count = %d\n", ls_world_rank, ls_recv_counter);
//                fflush(stdout);
////#endif
//            } 
            shadow_leap(1);
        }
//        pthread_mutex_unlock(&mt_mutex);
        PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
    }
    else{
        if(leap_flag && ls_recv_counter == ls_leap_recv_counter){
#ifdef DEBUG
            printf("[%d] Started to leap at recv count = %d\n", ls_world_rank, ls_recv_counter);
            fflush(stdout);
#endif
            shadow_leap(1);
        }
    } 
    rl_add(request, buf);
    ls_recv_counter++;

#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Irecv, time is %.6f, recv count = %d\n", ls_world_rank, diff, ls_recv_counter);
#else
    printf("[%d] end Irecv, recv count = %d\n", ls_world_rank, ls_recv_counter);
#endif
#endif
 
    return rc;
}

/* For request of receiving, the sync between main and shadow is implicitly
 * forced by the fact that main needs to forward the msg to its shadow;
 * for request of sending, no sync is forced (i.e. shadow may get ahead of main) 
 * because blocking is used for confirmation that user buffer is free to be 
 * reused, and the user buffer is not used at all at the shadow
 */
int MPI_Wait(MPI_Request *request, MPI_Status *status){
    req_list *p = NULL;
    MPI_Status temp_status;
    int length;

#ifdef DEBUG
    printf("[%d] begin MPI_Wait\n", ls_world_rank);
    fflush(stdout);
#endif

    if(ls_my_category == 0){
        if(status == MPI_STATUS_IGNORE){
            status = &temp_status;
        }
        PMPI_Wait(request, status);  
        if((p = rl_find(request)) != NULL){
            /*This requset is for MPI_Irecv()*/
            MPI_Get_count(status, MPI_CHAR, &length);
            socket_send(p->buf, length, status->MPI_SOURCE, status->MPI_TAG);
            rl_remove(p);
            p = NULL;
#ifdef DEBUG
            printf("[%d] end MPI_Wait, msg received with src = %d, tag = %d, length = %d \n", 
                    ls_world_rank, status->MPI_SOURCE, status->MPI_TAG, length);
            fflush(stdout);
#endif
        }
#ifdef DEBUG
        else{
            printf("[%d] end MPI_Wait, send completed\n", ls_world_rank);
            fflush(stdout);
        }
#endif
    }
    else{
        if((p = rl_find(request)) != NULL){
            /*This request is for MPI_Irecv()*/
            int temp_src;
            int temp_tag;
    
            mq_pop(&temp_src, &temp_tag, &length, p->buf);
            if(status != MPI_STATUS_IGNORE){
                status->MPI_SOURCE = temp_src;
                status->MPI_TAG = temp_tag;
                status->_ucount = length;
            }
            rl_remove(p);
            p = NULL;
#ifdef DEBUG
            printf("[%d] end MPI_Wait, msg received with src = %d, tag = %d, length = %d \n", 
                    ls_world_rank, temp_src, temp_tag, length);
            fflush(stdout);
#endif
        }
#ifdef DEBUG
        else{
            printf("[%d] end MPI_Wait, send completed\n", ls_world_rank);
            fflush(stdout);
        }
#endif
    }

    return MPI_SUCCESS; 
}

int MPI_Waitany(int count, MPI_Request array_of_requests[],
            int *index, MPI_Status *status){
    req_list *p = NULL;
    MPI_Status temp_status;
    int length;

#ifdef DEBUG
    printf("[%d] begin MPI_Waitany\n", ls_world_rank);
    fflush(stdout);
#endif

    if(ls_my_category == 0){
        if(status == MPI_STATUS_IGNORE){
            status = &temp_status;
        }
        PMPI_Waitany(count, array_of_requests, index, status);
        PMPI_Send(index, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_WAITANY_TAG, ls_cntr_world_comm);
        ls_cntr_msg_count++;
        if(*index != MPI_UNDEFINED && (p = rl_find(&array_of_requests[*index])) != NULL){
            MPI_Get_count(status, MPI_CHAR, &length);
            socket_send(p->buf, length, status->MPI_SOURCE, status->MPI_TAG);
            rl_remove(p);
            p = NULL;
#ifdef DEBUG
            printf("[%d] end MPI_Waitany, index = %d, msg received with src = %d, tag = %d, length = %d\n", 
                   ls_world_rank, *index, status->MPI_SOURCE, status->MPI_TAG, length);
            fflush(stdout);
#endif 
        }
#ifdef DEBUG
        else if(*index == MPI_UNDEFINED){
            printf("[%d] end MPI_Waitany, index == MPI_UNDEFINED\n", ls_world_rank);
            fflush(stdout);
        }
        else{
            printf("[%d] end MPI_Waitany, index = %d, send completed\n", ls_world_rank, *index);
            fflush(stdout);
        }
#endif
    }
    else{
        PMPI_Recv(index, 1, MPI_INT, ls_app_rank, SHADOW_WAITANY_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        ls_cntr_msg_count++;
        if(*index != MPI_UNDEFINED && (p = rl_find(&array_of_requests[*index])) != NULL){
            int temp_src;
            int temp_tag;

            mq_pop(&temp_src, &temp_tag, &length, p->buf);
            if(status != MPI_STATUS_IGNORE){
                status->MPI_SOURCE = temp_src;
                status->MPI_TAG = temp_tag;
                status->_ucount = length;
            }
            rl_remove(p);
            p = NULL;
#ifdef DEBUG
            printf("[%d] end MPI_Waitany, index = %d, msg received with src = %d, tag = %d, length = %d\n", 
                   ls_world_rank, *index, temp_src, temp_tag, length);
            fflush(stdout);
#endif 
        }
#ifdef DEBUG
        else if(*index == MPI_UNDEFINED){
            printf("[%d] end MPI_Waitany, index == MPI_UNDEFINED\n", ls_world_rank);
            fflush(stdout);
        }
        else{
            printf("[%d] end MPI_Waitany, index = %d, send completed\n", ls_world_rank, *index);
            fflush(stdout);
        }
#endif
    }

    return MPI_SUCCESS;
}
 
int MPI_Waitsome(int incount, MPI_Request array_of_requests[],
            int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]){
    req_list *p = NULL;
    int status_flag;
    int length;
    int *temp_buf = NULL; 
    int i;

#ifdef DEBUG
    printf("[%d] begin MPI_Waitsome\n", ls_world_rank);
    fflush(stdout);
#endif

    if(ls_my_category == 0){
        if(array_of_statuses == MPI_STATUSES_IGNORE){
            status_flag = 1;
            array_of_statuses = (MPI_Status *)malloc(incount * sizeof(MPI_Status));
        }
        else{
            status_flag = 0;
        }
 
        PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
        if(*outcount == MPI_UNDEFINED){
            PMPI_Send(outcount, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_WAITSOME_TAG, ls_cntr_world_comm);
            ls_cntr_msg_count++;
#ifdef DEBUG
            printf("[%d] end MPI_Waitsome, outcount == MPI_UNDEFINED\n", ls_world_rank);
            fflush(stdout);
#endif
        }
        else{
            temp_buf = (int *)malloc((*outcount + 1) * sizeof(int));
            temp_buf[0] = *outcount;
            for(i = 0; i < *outcount; i++){
                temp_buf[i+1] = array_of_indices[i];
            }
            PMPI_Send(temp_buf, *outcount + 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_WAITSOME_TAG, ls_cntr_world_comm);
            ls_cntr_msg_count++;
            for(i = 0; i < *outcount; i++){
                int index = array_of_indices[i];

                if((p = rl_find(&array_of_requests[index])) != NULL){
                    MPI_Get_count(&array_of_statuses[i], MPI_CHAR, &length);
                    socket_send(p->buf, length, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG);
                    rl_remove(p);
                    p = NULL;
#ifdef DEBUG
                    printf("[%d] MPI_Waitsome, outcount = %d, index = %d, msg_received with src = %d, tag = %d, length = %d\n", 
                        ls_world_rank, *outcount, index, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG,
                        length);
                    fflush(stdout);
#endif
                }                    
            }
            free(temp_buf); 
            if(status_flag)
                free(array_of_statuses);
        }
    }
    else{
        temp_buf = (int *)malloc((incount + 1) * sizeof(int));
        PMPI_Recv(temp_buf, incount + 1, MPI_INT, ls_app_rank, SHADOW_WAITSOME_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        ls_cntr_msg_count++;
        *outcount = temp_buf[0];
        if(temp_buf[0] == MPI_UNDEFINED){
#ifdef DEBUG
            printf("[%d] end MPI_Waitsome, outcount = MPI_UNDEFINED\n", ls_world_rank);
            fflush(stdout);
#endif
        }
        else{
            int index, temp_src, temp_tag;

            for(i = 0; i < *outcount; i++){
                index = temp_buf[i+1];
                array_of_indices[i] = index;
                if((p = rl_find(&array_of_requests[index])) != NULL){
                    mq_pop(&temp_src, &temp_tag, &length, p->buf);
#ifdef DEBUG
                    printf("[%d] MPI_Waitsome, outcount = %d, index = %d, msg_received with src = %d, tag = %d, length = %d\n", 
                        ls_world_rank, *outcount, index, temp_src, temp_tag, length);
                    fflush(stdout);
#endif
                    if(array_of_statuses != MPI_STATUSES_IGNORE){
                        array_of_statuses[i].MPI_SOURCE = temp_src;
                        array_of_statuses[i].MPI_TAG = temp_tag;
                        array_of_statuses[i]._ucount = length;
                    }
                    rl_remove(p);
                    p = NULL;
                }
            }
        }    
    }

#ifdef DEBUG
    printf("[%d] end MPI_Waitsome\n", ls_world_rank);
    fflush(stdout);
#endif

    return MPI_SUCCESS;
}

int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status *array_of_statuses){
#ifdef TIME_DEBUG
    struct timeval time_1, time_2;
    gettimeofday(&time_1, 0x0);
#endif
 
    int i;
#ifdef DEBUG
    printf("[%d] begin MPI_Waitall, %d requests in total\n", ls_world_rank, count);
#endif
    for(i = 0; i < count; i++){
        if(array_of_statuses == MPI_STATUSES_IGNORE){
#ifdef DEBUG
            printf("[%d] MPI_Waitall using MPI_STATUSES_IGNORE\n", ls_world_rank);
            fflush(stdout);
#endif
            MPI_Wait(&array_of_requests[i], MPI_STATUS_IGNORE);
        }
        else{
#ifdef DEBUG
            printf("[%d] MPI_Waitall with MPI_Statuses\n", ls_world_rank);
            fflush(stdout);
#endif
            MPI_Wait(&array_of_requests[i], &array_of_statuses[i]);
        }
    }
#ifdef DEBUG
#ifdef TIME_DEBUG
    gettimeofday(&time_2, 0x0);
    double sec = (time_2.tv_sec - time_1.tv_sec);
    double usec = (time_2.tv_usec - time_1.tv_usec) / 1000000.0;
    double diff = sec + usec;
    printf("[%d] end Waitall, time is %.6f\n", ls_world_rank, diff);
#else
    printf("[%d] end Waitall\n", ls_world_rank);
#endif
#endif
 
    return MPI_SUCCESS;
}

/* consistenty is guaranteed by having the main forward its flag
 * to shadow; if the request is for receiving, the main will forward 
 * the received msg to shadow once flag is positive
 */
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status){
    req_list *p = NULL;
    MPI_Status temp_status;
    int length;

#ifdef DEBUG
    printf("[%d] begin MPI_Test\n", ls_world_rank);
    fflush(stdout);
#endif

    if(ls_my_category == 0){
        if(status == MPI_STATUS_IGNORE){
            status = &temp_status;
        }
        PMPI_Test(request, flag, status); 
        PMPI_Send(flag, 1, MPI_INT, ls_world_rank + ls_app_size, SHADOW_TEST_TAG, ls_cntr_world_comm);  
        ls_cntr_msg_count++;
        if(*flag && ((p = rl_find(request)) != NULL)){
            /*This requset is for MPI_Irecv()*/
            MPI_Get_count(status, MPI_CHAR, &length);
            socket_send(p->buf, length, status->MPI_SOURCE, status->MPI_TAG);
            rl_remove(p);
            p = NULL;
#ifdef DEBUG
            printf("[%d] end MPI_Test, msg received with src = %d, tag = %d, length = %d \n", 
                ls_world_rank, status->MPI_SOURCE, status->MPI_TAG, length);
            fflush(stdout);
#endif
        }
#ifdef DEBUG
        else if(*flag){
            printf("[%d] end MPI_Test, send completed\n", ls_world_rank);
            fflush(stdout);
        }
        else if((p = rl_find(request)) != NULL){
            printf("[%d] end MPI_Test, msg not received yet\n", ls_world_rank);
            fflush(stdout);
        }
        else{
            printf("[%d] end MPI_Test, send not completed yet\n", ls_world_rank);
            fflush(stdout);
        }
#endif
    }
    else{
        PMPI_Recv(flag, 1, MPI_INT, ls_app_rank, SHADOW_TEST_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        ls_cntr_msg_count++;
        if(*flag && ((p = rl_find(request)) != NULL)){
            /*This request is for MPI_Irecv()*/
            int temp_src;
            int temp_tag;
    
            mq_pop(&temp_src, &temp_tag, &length, p->buf);
            if(status != MPI_STATUS_IGNORE){
                status->MPI_SOURCE = temp_src;
                status->MPI_TAG = temp_tag;
                status->_ucount = length;
            }
            rl_remove(p);
            p = NULL;
#ifdef DEBUG
            printf("[%d] end MPI_Test, msg received with src = %d, tag = %d, length = %d \n", 
                ls_world_rank, status->MPI_SOURCE, status->MPI_TAG, length);
            fflush(stdout);
#endif
        }
#ifdef DEBUG
        else if(*flag){
            printf("[%d] end MPI_Test, send completed\n", ls_world_rank);
            fflush(stdout);
        }
        else if((p = rl_find(request)) != NULL){
            printf("[%d] end MPI_Test, msg not received yet\n", ls_world_rank);
            fflush(stdout);
        }
        else{
            printf("[%d] end MPI_Test, send not completed yet\n", ls_world_rank);
            fflush(stdout);
        }
#endif
    }

    return MPI_SUCCESS; 
}


int MPI_Testall(int count, MPI_Request array_of_requests[],
            int *flag, MPI_Status array_of_statuses[]){
    req_list *p = NULL;
    int length;
    int status_flag;
    int i;

#ifdef DEBUG
    printf("[%d] begin MPI_Testall\n", ls_world_rank);
    fflush(stdout);
#endif

    if(ls_my_category == 0){
        if(array_of_statuses == MPI_STATUSES_IGNORE){
            status_flag = 1;
            array_of_statuses = (MPI_Status *)malloc(count * sizeof(MPI_Status));
        }
        else{
            status_flag = 0;
        }
        PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
        PMPI_Send(flag, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_TESTALL_TAG, ls_cntr_world_comm);
        ls_cntr_msg_count++;
        if(*flag){
            for(i = 0; i < count; i++){
                if((p = rl_find(&array_of_requests[i])) != NULL){
                    MPI_Get_count(&array_of_statuses[i], MPI_CHAR, &length);
                    socket_send(p->buf, length, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG);
                    rl_remove(p);
                    p = NULL;
#ifdef DEBUG
                    printf("[%d] MPI_Testall, msg received with src = %d, tag = %d, length = %d\n", 
                            ls_world_rank, array_of_statuses[i].MPI_SOURCE, array_of_statuses[i].MPI_TAG, length);
                    fflush(stdout);
#endif
                }
#ifdef DEBUG
                else{
                    printf("[%d] MPI_Testall, send completed\n", ls_world_rank);
                    fflush(stdout);
                }
#endif
            }
        } 
#ifdef DEBUG
        else{
            printf("[%d] end MPI_Testall, not completed yet\n", ls_world_rank);
            fflush(stdout);
        }
#endif
        if(status_flag)
            free(array_of_statuses);

    }
    else{
        PMPI_Recv(flag, 1, MPI_INT, ls_app_rank, SHADOW_TEST_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        ls_cntr_msg_count++;
        if(*flag){
            for(i = 0; i < count; i++){
                if((p = rl_find(&array_of_requests[i])) != NULL){
                    int temp_src;
                    int temp_tag;
                    
                    mq_pop(&temp_src, &temp_tag, &length, p->buf);
                    if(array_of_statuses != MPI_STATUSES_IGNORE){
                        array_of_statuses[i].MPI_SOURCE = temp_src;
                        array_of_statuses[i].MPI_TAG= temp_tag;
                        array_of_statuses[i]._ucount = length;
                    }
                    rl_remove(p);
                    p = NULL;
#ifdef DEBUG
                    printf("[%d] MPI_Testall, msg received with src = %d, tag = %d, length = %d\n", 
                            ls_world_rank, temp_src, temp_tag, length);
                    fflush(stdout);
#endif
                }
#ifdef DEBUG
                else{
                    printf("[%d] MPI_Testall, send completed\n", ls_world_rank);
                    fflush(stdout);
                }
#endif
            }
        }
#ifdef DEBUG
        else{
            printf("[%d] end MPI_Testall, not completed yet\n", ls_world_rank);
            fflush(stdout);
        }
#endif
    }

    return MPI_SUCCESS;
}



int MPI_Group_size(MPI_Group group, int *size){
    return PMPI_Group_size(group, size);
}

int MPI_Group_rank(MPI_Group group, int *rank){
    return PMPI_Group_rank(group, rank);
}

int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result){
    return PMPI_Group_compare(group1, group2, result);
}

int MPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[]){
    return PMPI_Group_translate_ranks(group1, n, ranks1, group2, ranks2);
}

int MPI_Comm_group(MPI_Comm comm, MPI_Group *group){
    if(comm == MPI_COMM_WORLD){
        return PMPI_Comm_group(ls_data_world_comm, group);
    }
    else{
        return PMPI_Comm_group(comm, group);
    }
}

int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup){
    return PMPI_Group_union(group1, group2, newgroup);
}

int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup){
    return PMPI_Group_intersection(group1, group2, newgroup);
}

int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup){
    return PMPI_Group_difference(group1, group2, newgroup);
}

int MPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup){
    return PMPI_Group_incl(group, n, ranks, newgroup); 
}

int MPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup){
    return PMPI_Group_excl(group, n, ranks, newgroup); 
}

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup){
    return PMPI_Group_range_incl(group, n, ranges, newgroup);
}

int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup){
    return PMPI_Group_range_excl(group, n, ranges, newgroup);
}

int MPI_Group_free(MPI_Group *group){
    return PMPI_Group_free(group);
}


int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm){
#ifdef DEBUG
    printf("[%d] begin Comm_split\n", ls_world_rank);
#endif
    if(comm == MPI_COMM_WORLD){
        return PMPI_Comm_split(ls_data_world_comm, color, key, newcomm);
    }
    else{
        return PMPI_Comm_split(comm, color, key, newcomm);
    }
} 


int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm){
    if(comm == MPI_COMM_WORLD)
        return PMPI_Comm_dup(ls_data_world_comm, newcomm);
    else
        return PMPI_Comm_dup(comm, newcomm);
}


int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result){
    if(comm1 == MPI_COMM_WORLD)
        comm1 = ls_data_world_comm;
    if(comm2 == MPI_COMM_WORLD)
        comm2 = ls_data_world_comm;

    return PMPI_Comm_compare(comm1, comm2, result);
}
    

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm){
    if(comm == MPI_COMM_WORLD)
        return PMPI_Comm_create(ls_data_world_comm, group, newcomm);
    else
        return PMPI_Comm_create(comm, group, newcomm);
}

int MPI_Comm_free(MPI_Comm *comm){
    return PMPI_Comm_free(comm);
}


//int MPI_Comm_test_inter(MPI_Comm comm, int *flag){
//    return PMPI_Comm_test_inter(comm, flag);
//}
//
//
//int MPI_Comm_remote_size(MPI_Comm comm, int *size){
//    return PMPI_Comm_remote_size(comm, size);
//}
//
//int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group){
//    return PMPI_Comm_remote_group(comm, group);
//}
//
//int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader,
//            MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm){
//    return PMPI_Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
//}
//
///*MPI_Intercomm_merge assumes that one group uses high=true and one group uses high=false
// * Won't work if two group use the same value for high
// */
//int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm){
//    MPI_Comm tempcomm;
//    int rc;
//    MPI_Group merge_group, first_group, second_group, group_all;
//    int first_group_size, second_group_size, group_all_size;
//    int first_range[2][3], second_range[2][3];
//
//    PMPI_Intercomm_merge(intercomm, high, &tempcomm);
//    MPI_Comm_size(tempcomm, &group_all_size);
//    MPI_Comm_group(tempcomm, &merge_group);
//    if(!high){
//        MPI_Comm_size(intercomm, &first_group_size);
//        MPI_Comm_remote_size(intercomm, &second_group_size);
//    }
//    else{
//        MPI_Comm_remote_size(intercomm, &first_group_size);
//        MPI_Comm_size(intercomm, &second_group_size);
//    }
//    printf("My id is %d, first group size is %d, second group size is %d, all group size is %d\n", ls_world_rank, first_group_size, second_group_size, group_all_size);
////    if(!high){
////        MPI_Comm_group(intercomm, &first_group);
////        MPI_Comm_remote_group(intercomm, &second_group);
////    }
////    else{
////        MPI_Comm_group(intercomm, &second_group);
////        MPI_Comm_remote_group(intercomm, &first_group);
////    }
//    first_range[0][0] = 0;
//    first_range[0][1] = first_group_size - 1;
//    first_range[0][2] = 1; 
//    first_range[1][0] = 2 * first_group_size;
//    first_range[1][1] = 2 * first_group_size + second_group_size - 1;
//    first_range[1][2] = 1;
//    second_range[0][0] = first_group_size;
//    second_range[0][1] = 2 * first_group_size - 1;
//    second_range[0][2] = 1;
//    second_range[1][0] = 2 * first_group_size + second_group_size;
//    second_range[1][1] = 2 * first_group_size + 2 * second_group_size - 1;
//    second_range[1][2] = 1;
//    PMPI_Group_range_incl(merge_group, 2, first_range, &first_group);
//    PMPI_Group_range_incl(merge_group, 2, second_range, &second_group); 
//    PMPI_Group_union(first_group, second_group, &group_all);
//    rc = PMPI_Comm_create(tempcomm, group_all, newintracomm);
////    MPI_Group_union(first_group, second_group, &group_all);
////    rc = MPI_Comm_create(MPI_COMM_WORLD, group_all, newintracomm);
//    return rc;
//}




int MPI_Dims_create(int nnodes, int ndims, int dims[]){
    return PMPI_Dims_create(nnodes, ndims, dims);
}


int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[],
            const int periods[], int reorder, MPI_Comm *comm_cart){
    if(comm_old == MPI_COMM_WORLD)
        return PMPI_Cart_create(ls_data_world_comm, ndims, dims, periods, reorder, comm_cart);
    else
        return PMPI_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart);
}


int MPI_Topo_test(MPI_Comm comm, int *status){
    return PMPI_Topo_test(comm, status);
}


int MPI_Cartdim_get(MPI_Comm comm, int *ndims){
    return PMPI_Cartdim_get(comm, ndims);
}


int MPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[],
            int coords[]){
    return PMPI_Cart_get(comm, maxdims, dims, periods, coords);
}
        

int MPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank){
    return PMPI_Cart_rank(comm, coords, rank);
}


int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims,
            int coords[]){
    return PMPI_Cart_coords(comm, rank, maxdims, coords);
}


int MPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *comm_new){
    return PMPI_Cart_sub(comm, remain_dims, comm_new);
}


int MPI_Cart_shift(MPI_Comm comm, int direction, int disp,
            int *rank_source, int *rank_dest){
    return PMPI_Cart_shift(comm, direction, disp, rank_source, rank_dest);
}
    


//int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[],
//            const int periods[], int *newrank){
//    int rc;
//
//    if(ls_my_category == 0){
//        rc = PMPI_Cart_map(comm, ndims, dims, periods, newrank);
//        //forward result to my shadow
//        PMPI_Send(newrank, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_CART_NEWRANK_TAG, ls_cntr_world_comm);
//    }
//    else{
//        rc = PMPI_Recv(newrank, 1, MPI_INT, ls_app_rank, SHADOW_CART_NEWRANK_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
//    } 
//    
//    return rc;
//}
//
//int MPI_Graph_map(MPI_Comm comm, int nnodes, const int index[],
//            const int edges[], int *newrank){
//    int rc;
//
//    if(ls_my_category == 0){
//        rc = PMPI_Graph_map(comm, nnodes, index, edges, newrank);
//        //forward result to my shadow
//        PMPI_Send(newrank, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_CART_NEWRANK_TAG, ls_cntr_world_comm);
//    }
//    else{
//        rc = PMPI_Recv(newrank, 1, MPI_INT, ls_app_rank, SHADOW_CART_NEWRANK_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
//    } 
//    
//    return rc;
//}
//
//
//
int MPI_Graph_create(MPI_Comm comm_old, int nnodes, const int index[],
            const int edges[], int reorder, MPI_Comm *comm_graph){
    if(comm_old == MPI_COMM_WORLD)
        return PMPI_Graph_create(ls_data_world_comm, nnodes, index, edges, reorder, comm_graph);
    else
        return PMPI_Graph_create(comm_old, nnodes, index, edges, reorder, comm_graph);
}

int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges){
    //comm should be created by MPI_Graph_create, thus should not be MPI_COMM_WORLD
    return PMPI_Graphdims_get(comm, nnodes, nedges);
}

int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int *index, int *edges){
    return PMPI_Graph_get(comm, maxindex, maxedges, index, edges);
}

int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors){
    return PMPI_Graph_neighbors_count(comm, rank, nneighbors);
}

int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors,
            int neighbors[]){
    return PMPI_Graph_neighbors(comm, rank, maxneighbors, neighbors);
}

