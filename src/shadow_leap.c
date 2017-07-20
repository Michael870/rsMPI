#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "mpi.h"
#include "shared.h"
#include "mpi_override.h"
#include "shadow_leap.h"
#include "msg_queue.h"
#include <unistd.h> 

#define DEBUG

/*global varibales*/
int leap_state_count = 0;
state_data *leap_list_head = NULL;
state_data *leap_list_tail = NULL;
struct timespec reboot_time;
int leap_loop_count = -1; //a counter to track current loop index
//int leap_point_sent = 0;
//int leap_enabled = 0; //a switch that should be set only after entering main loop
//int ls_leap_buf_size = 160000;



int ls_sync_main_shadow(){
    int buf = 0;

    if(ls_my_category == 0){
        PMPI_Send(&buf, 1, MPI_INT, ls_world_rank + ls_app_size, SHADOW_SYNC_TAG, ls_cntr_world_comm);
        PMPI_Recv(&buf, 1, MPI_INT, ls_world_rank + ls_app_size, SHADOW_SYNC_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
    }
    else{
        PMPI_Recv(&buf, 1, MPI_INT, ls_world_rank - ls_app_size, SHADOW_SYNC_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        PMPI_Send(&buf, 1, MPI_INT, ls_world_rank - ls_app_size, SHADOW_SYNC_TAG, ls_cntr_world_comm);
    }
#ifdef DEBUG
    printf("[%d] main and shadow synced\n", ls_world_rank);
#endif
    return 0;
} 

//void main_force_leaping(){
//    if(ls_my_category == 0){
//        int code = -2;
//
//        leap_flag = 1;
//        PMPI_Send(&code, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
//#ifdef DEBUG
//        printf("[%d] notified SC to leap\n", ls_world_rank);
//        fflush(stdout);
//#endif
//    }
//    shadow_leap(1);
//}

void shadow_force_leaping(double buf_util){
    int code;
    int i;

    leap_flag = 1;
#ifdef COORDINATED_LEAPING
    PMPI_Send(&code, 1, MPI_INT, 0, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm);
#else
    PMPI_Send(&code, 1, MPI_INT, ls_app_rank, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm);
#endif
#ifdef DEBUG
#ifdef COORDINATED_LEAPING
    printf("[%d] Detected buffer reaching leaping threshold, utilization is %.2f. Notified main leader (0) to leap.\n",
            ls_world_rank, buf_util);
#else
    printf("[%d] Detected buffer reaching leaping threshold, utilization is %.2f. Notified main to leap.\n",
            ls_world_rank, buf_util);
#endif
    fflush(stdout);
#endif
//    PMPI_Send(&code, 1, MPI_INT, ls_app_rank, SHADOW_FORCE_LEAPING_TAG, ls_cntr_world_comm);
//    printf("[%d] Detected buffer reaching leaping threshold, utilization is %.2f. Notified main to leap.\n",
//            ls_world_rank, buf_util);
//    fflush(stdout);

}

void leap_register_state(void *addr, int count, MPI_Datatype dt){
#ifdef DEBUG
    MPI_Aint lb, extent;
    MPI_Type_get_extent(dt, &lb, &extent);
#endif
    state_data *temp = (state_data *)malloc(sizeof(state_data));
    temp->addr = addr;
    temp->count = count;
    temp->dt = dt;
    temp->next = NULL;
    if(leap_state_count == 0){
        leap_list_head = leap_list_tail = temp;
    }
    else{
        leap_list_tail->next = temp;
        leap_list_tail = temp;
    }
#ifdef DEBUG
    //if(ls_world_rank < 2){
        printf("[%d] registered 1 data (size = %d)\n", ls_world_rank, count*extent);
        fflush(stdout);
    //}
#endif
    leap_state_count++;
}


void shadow_leap(int type){
    int code;
    double start_t, end_t;

    if(type == 0){
#ifdef DEBUG
        printf("[%d] shadow_leap from shadow to main, loop counter = %d\n", ls_world_rank, leap_loop_count);
        fflush(stdout);
#endif
        start_t = MPI_Wtime();
        state_data *p = leap_list_head;
        if(ls_my_category == 1){
            while(p){
                PMPI_Send(p->addr, p->count, p->dt, ls_world_rank - ls_app_size, SHADOW_LEAP_TAG, ls_cntr_world_comm);
                p = p->next;
            }
#ifdef DEBUG
            printf("[%d] Sent state to process %d\n", ls_world_rank, ls_world_rank - ls_app_size);
            fflush(stdout);
#endif
            code = -3;
            PMPI_Send(&code, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
#ifdef DEBUG
            printf("[%d] notified my SC to resume suspended shadows\n", ls_world_rank);
            fflush(stdout); 
#endif
        } 
        else{
            while(p){    
                PMPI_Recv(p->addr, p->count, p->dt, ls_world_rank + ls_app_size, SHADOW_LEAP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
                p = p->next;
            }
#ifdef DEBUG
            printf("[%d] Recv state from process %d\n", ls_world_rank, ls_world_rank + ls_app_size);
            fflush(stdout);
#endif
        }
        end_t = MPI_Wtime();
#ifdef DEBUG
        printf("[%d] leaping took %.2f seconds\n", ls_world_rank, end_t - start_t);
#endif
    }
    else{ /*leaping from main to shadow*/
#ifdef DEBUG
        printf("[%d] shadow_leap from main to shadow, loop counter = %d, data msg counter = %d, cntr msg counter = %d\n", ls_world_rank, leap_loop_count, ls_data_msg_count, ls_cntr_msg_count);
        fflush(stdout);
#endif
        start_t = MPI_Wtime();
        //firstly, transfer state
        state_data *p = leap_list_head;
        if(ls_my_category == 0){
            while(p){
                PMPI_Send(p->addr, p->count, p->dt, ls_world_rank + ls_app_size, SHADOW_LEAP_TAG, ls_cntr_world_comm);
                p = p->next;
            }
#ifdef DEBUG
            printf("[%d] Sent state to process %d\n", ls_world_rank, ls_world_rank + ls_app_size);
            fflush(stdout);
#endif
        } 
        else{
            while(p){    
                PMPI_Recv(p->addr, p->count, p->dt, ls_world_rank - ls_app_size, SHADOW_LEAP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
                p = p->next;
            }
#ifdef DEBUG
            printf("[%d] Recv state from process %d\n", ls_world_rank, ls_world_rank - ls_app_size);
            fflush(stdout);
#endif
        }
        //secondly, consume obsolete msgs
        if(ls_my_category == 0){
            int temp[3];
            temp[0] = leap_loop_count;
            temp[1] = ls_data_msg_count;
            temp[2] = ls_cntr_msg_count;
            PMPI_Send(temp, 3, MPI_INT, ls_world_rank + ls_app_size, SHADOW_LEAP_TAG, ls_cntr_world_comm);
        }
        else{
            int temp[3];
            int count_diff = 0;
            int i;
            MPI_Status temp_status;
            char buffer[128];
            int length;

            PMPI_Recv(temp, 3, MPI_INT, ls_world_rank - ls_app_size, SHADOW_LEAP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
#ifdef DEBUG
            printf("[%d] main loop count = %d, data msg count = %d, cntr msg count = %d, shadow loop count = %d, data msg count = %d, cntr msg count = %d\n", ls_world_rank, temp[0], temp[1], temp[2], leap_loop_count, ls_data_msg_count, ls_cntr_msg_count);
#endif
            /*consume data msg*/
            while(shared_mq->count < temp[1] - ls_data_msg_count){
                printf("[%d] Waiting for more messages, data msg count at main = %d, data msg count at shadow = %d, mq count = %d\n", ls_world_rank, temp[1], ls_data_msg_count, shared_mq->count);
                sleep(1);
            }
            mq_clear(temp[1] - ls_data_msg_count);
            for(i = 0; i < temp[2] - ls_cntr_msg_count; i++){
                
                PMPI_Probe(ls_app_rank, MPI_ANY_TAG, ls_cntr_world_comm, &temp_status);
                PMPI_Get_count(&temp_status, MPI_CHAR, &length); 
                if(length < 128){
                    PMPI_Recv(buffer, length, MPI_CHAR, ls_app_rank, temp_status.MPI_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
                }
                else{
                    char *buffer = malloc(length);
                    
                    PMPI_Recv(buffer, length, MPI_CHAR, ls_app_rank, temp_status.MPI_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
                    free(buffer);
                } 
            }
#ifdef DEBUG
            printf("[%d] Cleared control messages\n", ls_world_rank);
            fflush(stdout);
#endif

            leap_loop_count = temp[0]; //update shadow's loop count
            ls_data_msg_count = temp[1];
            ls_cntr_msg_count = temp[2];
        }
#ifdef DEBUG
        end_t = MPI_Wtime();
        printf("[%d] leaping took %.2f seconds\n", ls_world_rank, end_t - start_t);
#endif
    }
    leap_flag = 0;
    //leap_point_sent = 0;
}

void ls_inject_failure(int rank, int iter){
    if(ls_my_category == 0){
        if(leap_loop_count == iter){
            if(ls_world_rank == rank){
                int code = ls_world_rank;

                printf("[%d]: I am going to have a failure!\n", ls_world_rank);
                fflush(stdout);
                PMPI_Send(&code, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
                //simulate reboot time
                reboot_time.tv_sec = REBOOT_TIME;
                reboot_time.tv_nsec = 0;
                nanosleep(&reboot_time, NULL);
#ifdef DEBUG
                printf("[%d] Finished rebooting (%d secs)\n", ls_world_rank, reboot_time.tv_sec);
                fflush(stdout);
#endif
                shadow_leap(0); // shadow to main leaping
            }
            else{
                int code = -2;

                PMPI_Send(&code, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
#ifdef DEBUG
                printf("[%d] Detected failure of main %d. Notified SC to initiate leaping\n", ls_world_rank, rank);
                fflush(stdout);
#endif
//                PMPI_Send(&ls_recv_counter, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_RECV_POINT_TAG, ls_cntr_world_comm);
//                //leap_point_sent = 1;
//#ifdef DEBUG
//                printf("[%d] Notified shadow of my recv count = %d\n", ls_world_rank, ls_recv_counter);
//                fflush(stdout);
//#endif
                leap_flag = 1;
//                shadow_leap(1); // main to shadow leaping
            }
        }
    }
    else{
        if(ls_app_rank == rank){ //I am the shadow of the failing main
            if(leap_loop_count == iter){
                shadow_leap(0); //shadow to main leaping
            }
        }
//        else if(leap_flag){
//            shadow_leap(1);
//        }
    }
//    if(ls_world_rank == rank && leap_loop_count == iter){
//        int i;
//        int code = 0;
//        int temp[2];
//
//        printf("[%d]: I am going to have a failure!\n", ls_world_rank);
//        fflush(stdout);
//        ////////////////////////to force leaping at the begining of loop
//        ls_my_main_failure_point = leap_loop_count;
//        /*notify other mains*/
//        code = ls_world_rank;
//        for(i = 0; i < ls_app_size; i++){
//            if(i != ls_world_rank){
//                PMPI_Send(&code, 1, MPI_INT, i, 0, ls_failure_world_comm);
//            }
//        }
//    
//        PMPI_Send(&code, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
//        ////////////////////////to force leaping at the begining of loop
//        temp[0] = leap_loop_count;
//        temp[1] = ls_recv_counter;
//        ls_leap_recv_counter = ls_recv_counter;
//        PMPI_Send(temp, 2, MPI_INT, ls_world_rank + ls_app_size, 0, ls_failure_world_comm); // notify my shadow of my failure point
//#ifdef DEBUG
//        printf("[%d] notified my shadow of my failure point\n", ls_world_rank);
//        fflush(stdout);
//#endif
//        //simulate reboot time
//        reboot_time.tv_sec = REBOOT_TIME;
//        reboot_time.tv_nsec = 0;
//        nanosleep(&reboot_time, NULL);
//        //////////////////removing leaping point in order to force leaping at fixed point
//    }
//    shadow_leap();
} 



void ls_reset_recv_counter(){
    ls_recv_counter = 0;
//    leap_enabled = 1;
    leap_loop_count++;
}







