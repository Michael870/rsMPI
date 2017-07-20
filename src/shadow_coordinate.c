#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "mpi_override.h"



/*communicate with mains and coordinate the shadows*/
void coordinate(int num){
//    int pids[ls_app_size];
    int i;
    MPI_Status status;
    int code;
    int remain_task_count = num;
//    int* task_status = (int*)malloc(num * sizeof(int));//0: incomplete, 1: completed
    int* pids = (int *)malloc(num * sizeof(int));;
    int my_sc_rank = ls_world_rank - 2 * ls_app_size;
    int my_main_start_rank = shadow_ratio * my_sc_rank;
    int my_shadow_start_rank = ls_app_size + my_main_start_rank;
    int group_finished = 1;
    int notified_all = 0;
    struct timespec check_time;
    
    check_time.tv_sec = 1;
    check_time.tv_nsec = 1000;

//    for(i = 0; i < num; i++){
//        task_status[i] = 0;
//    }

    /*collect pid from each shadow process*/
    for(i = 0; i < num; i++){
        int ret;
        ret = PMPI_Recv(&pids[i], 1, MPI_INT, my_shadow_start_rank + i, 0, ls_cntr_world_comm, &status);
        if(ret != MPI_SUCCESS){
            printf("[%d] Found failure before initialization. Going to abort!\n", ls_world_rank);
            //free(task_status);
            free(pids);
            MPI_Abort(ls_data_world_comm, -1);
        }
    }    
#ifdef DEBUG
    printf("[%d] Collected all pids\n", ls_world_rank);
    fflush(stdout);
#endif
    while(1){
        int ret;
        int msg_arrived = 0;

        while(!msg_arrived){
            PMPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, ls_cntr_world_comm, &msg_arrived, &status);
            if(!msg_arrived){
                nanosleep(&check_time, NULL);
            }
        }
        
        ret = PMPI_Recv(&code, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, ls_cntr_world_comm, &status);
        if(ret != MPI_SUCCESS)
            continue;
        if(code == -1){    /*task has finished*/
            if(is_main(status.MPI_SOURCE)){
//               if(!task_status[status.MPI_SOURCE - my_main_start_rank]){
                    remain_task_count--;
//                    task_status[status.MPI_SOURCE - my_main_start_rank] = 1;
//                }
#ifdef DEBUG
                printf("[%d]: got notified that Main %d has finished, remain_task_count = %d\n", ls_world_rank, status.MPI_SOURCE, remain_task_count);
                fflush(stdout); 
#endif
            }
            else{
                int main_rank = get_main_rank(status.MPI_SOURCE);
#ifdef DEBUG
                printf("[%d]: got notified that Shadow %d has finished, remain_task_count = %d\n", ls_world_rank, main_rank, remain_task_count);
                fflush(stdout); 
#endif
            }
            if(remain_task_count <= 0 && !notified_all){
                int i;

                //notified all other sc that I have finished
                for(i = 2*ls_app_size; i < ls_world_size; i++){
                    if(i == ls_world_rank)
                        continue;
                    PMPI_Send(&ls_world_rank, 1, MPI_INT, i, 0, ls_cntr_world_comm);
                }

                notified_all = 1;
            }
        }
        else if(code >= my_main_start_rank && code < my_main_start_rank + num){ /*main has failed*/
#ifdef DEBUG
                printf("[%d]: got notified that main %d has failed.\n", ls_world_rank, code);
                fflush(stdout);
#endif
                for(i = 0; i < num; i++){
                    if(i + my_main_start_rank != code){
                        /*suspend collocated shadows for speeding up*/
#ifdef DEBUG
                        printf("[%d] Suspended shadow %d\n", ls_world_rank, i + my_main_start_rank);
#endif
                        kill(pids[i], SIGSTOP);    
                    }
//                    else{
//                        kill(pids[i], SIGUSR2);
//#ifdef DEBUG
//                        printf("[%d] notified shadow %d of its main failure\n", ls_world_rank, i + my_main_start_rank);
//                        fflush(stdout);
//#endif
//                    }
                }
        }
        else if(code >= ls_app_size * 2 && code < ls_world_size){
            group_finished++;
#ifdef DEBUG
            printf("[%d] Got notified that SC group %d has finished\n", ls_world_rank, code);
            fflush(stdout);
#endif
        }
        else if(code == -2){ //code for leaping
            int main_rank = status.MPI_SOURCE;
            
            kill(pids[main_rank - my_main_start_rank], SIGTERM);
#ifdef DEBUG
            printf("[%d] notified Shadow %d to leap\n", ls_world_rank, main_rank);
            fflush(stdout);
#endif
        }
        else if (code == -3){ //code for resuming collocated shadows 
            int rank = status.MPI_SOURCE - ls_app_size;
            for(i = 0; i < num; i++){
                if(i + my_main_start_rank != rank){
                    /*resume collocated shadows*/
#ifdef DEBUG
                    printf("[%d] Resumed shadow %d\n", ls_world_rank, i + my_main_start_rank);
#endif
                    kill(pids[i], SIGCONT);    
                }
            }
#
        }
        else{
            printf("[%d] Unknown code for shadow coordinator! Code = %d, source = %d\n", ls_world_rank, code, status.MPI_SOURCE);
//            free(task_status);
            free(pids);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        if(group_finished >= num_of_sc && remain_task_count <= 0){
#ifdef DEBUG
            printf("[%d] All scs are finished\n", ls_world_rank);
            fflush(stdout);
#endif
            int i;
            for(i = 0; i < num; i++){
               kill(pids[i], SIGUSR1);
            }
//            free(task_status);
            free(pids);
            MPI_Finalize();
            exit(0);
        }
    }
}
