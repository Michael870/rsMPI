#include "mpi.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include "monitor_thread.h"
#include "shadow_leap.h"
#include "mpi_override.h"

#define DEBUG

//pthread_mutex_t ls_mutex;
pthread_t ls_monitor_thread, ls_compute_thread;
pthread_attr_t ls_monitor_thread_attr;

void free_thread_resources(){
    pthread_attr_destroy(&ls_monitor_thread_attr);
}

/*monitor thread for main process*/
void* monitor_thread_main(void* arg){
    int code;
    MPI_Status status;
    struct timespec check_time;
    
    check_time.tv_sec = 1;
    check_time.tv_nsec = 1000;


    while(1){
        int ret;
        int msg_arrived = 0;
        int code;
        int parent = (ls_app_rank - 1) / 2;
        int lchild = 2 * ls_app_rank + 1;
        int rchild = 2 * ls_app_rank + 2;

        while(!msg_arrived){
#ifdef COORDINATED_LEAPING
            PMPI_Iprobe(MPI_ANY_SOURCE, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm, &msg_arrived, &status);
#else
            PMPI_Iprobe(ls_app_rank + ls_app_size, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm, &msg_arrived, &status);
#endif
//            PMPI_Iprobe(ls_app_rank + ls_app_size, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm, &msg_arrived, &status);
            /*sleep for some time to relinquish CPU*/
            if(!msg_arrived){
                nanosleep(&check_time, NULL);
            }
        }
        PMPI_Recv(&code, 1, MPI_INT, status.MPI_SOURCE, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        printf("[%d] Helper thread got a msg from %d\n", ls_world_rank, status.MPI_SOURCE);
        msg_arrived = 0;
#ifdef COORDINATED_LEAPING
        if(ls_app_rank == 0){
            if(status.MPI_SOURCE < ls_app_size){
                printf("[%d] Main helper thread, recv a msg from %d. This should never happen\n", ls_world_rank, status.MPI_SOURCE);
                PMPI_Abort(MPI_COMM_WORLD, -1);
            }
        }
        else if(status.MPI_SOURCE != parent){
            printf("[%d] Main helper thread, recv a msg from %d. This should never happen\n", ls_world_rank, status.MPI_SOURCE);
            PMPI_Abort(MPI_COMM_WORLD, -1);
        }
        if(ls_app_rank != 0 || !leap_flag){
            if(lchild < ls_app_size){
                PMPI_Send(&code, 1, MPI_INT, lchild, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm);
            }
            if(rchild < ls_app_size){
                PMPI_Send(&code, 1, MPI_INT, rchild, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm);
            }
        }
#endif
        /*trigger forced leaping*/
        if(!leap_flag){
            leap_flag = 1;
            code = -2;
            PMPI_Send(&code, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
#ifdef DEBUG
            printf("[%d] Learnt forced leaping. Notified SC to initiate leaping\n", ls_world_rank);
            fflush(stdout);
#endif
        }    
    }
}



/*monitor thread for shadow process*/
void* monitor_thread_shadow(void* arg){
    int code;
    MPI_Status status;
    int probe_flag = 0;
    struct timespec stime;
    int i;

    wait_for_msg();
}

void launch_monitor_thread(int is_main){
    int rc;
    sigset_t set;

//    pthread_mutex_init(&ls_mutex, NULL);
    pthread_attr_init(&ls_monitor_thread_attr);
    pthread_attr_setdetachstate(&ls_monitor_thread_attr, PTHREAD_CREATE_JOINABLE);
    ls_compute_thread = pthread_self();
    
    if(is_main){
        //pthread_mutex_init(&mt_mutex, NULL);
        rc = pthread_create(&ls_monitor_thread, &ls_monitor_thread_attr, monitor_thread_main, NULL);
    }
    else{
        /*block signals for monitor thread*/
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        sigaddset(&set, SIGUSR2);
        sigaddset(&set, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        rc = pthread_create(&ls_monitor_thread, &ls_monitor_thread_attr, monitor_thread_shadow, NULL);
        /*unblock signals for compute thread*/
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    }
    if(rc){
        printf("[%d]: ERROR: return code from pthread_create() is %d!\n", ls_world_rank, rc);
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
#ifdef DEBUG
    printf("[%d] Launched monitor thread\n", ls_world_rank);
    fflush(stdout);
#endif
}


















