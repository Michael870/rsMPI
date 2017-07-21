#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "mpi.h"
#include "shared.h"
#include "mpi_override.h"
#include "monitor_thread.h"
#include "socket.h"
//#include "error_handler.h"
#include <unistd.h>


int op_is_commute(MPI_Op op){
    return 1;
}

/**
* Returns the cube dimension of a given value.
*
* @param value The integer value to examine
*
* @returns cubedim The smallest cube dimension containing that value
*
* Look at the integer "value" and calculate the smallest power of two
* dimension that contains that value.
*
* WARNING: *NO* error checking is performed.  This is meant to be a
* fast inline function 
*/
int opal_cube_dim(int value) 
{
    int dim, size;
    for (dim = 0, size = 1; size < value; ++dim, size <<= 1) {
        continue;
    }
    return dim;
}


int real_to_virtual(int rank, int root, int size){
    return (rank + size - root) % size;
}

int virtual_to_real(int rank, int root, int size){
    return (rank + root) % size;
}


void report_pid(){
    int my_pid = getpid();
    PMPI_Send(&my_pid, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
}


void free_resources(){
#ifdef DEBUG
    printf("[%d] begin free resources\n", ls_world_rank);
#endif
    if(ls_my_category < 2)
        PMPI_Comm_free(&ls_data_world_comm);
    PMPI_Comm_free(&ls_cntr_world_comm);
//    PMPI_Comm_free(&ls_failure_world_comm);
    //free(ls_msg_src);
    if(ls_my_category < 2){
#ifdef USE_RDMA
        rclose(sock_fd);
#else
        close(sock_fd);
#endif
    }
#ifdef DEBUG
    printf("[%d] end free resources\n", ls_world_rank);
#endif
 
}


int get_main_rank(int rank){
    if(rank < ls_app_size)
        return rank;
    else
        return rank % ls_app_size;
}
 
int is_main(int rank){
    if(rank < ls_app_size)
        return 1;
    else
        return 0;
}

int is_sc(int rank){
    if(rank < 2*ls_app_size)
        return 0;
    else
        return 1;
}


void read_config(){
    FILE* fp = fopen("./configure.txt", "r");

    if(fp == NULL){
        printf("Fail to read from configuration file!\n");
        exit(-1);
    }
 
    fscanf(fp, "%d", &shadow_ratio);
    if(shadow_ratio <= 0 || shadow_ratio > 5){
        printf("Wrong specification of shadow_ratio (=%d)!\n", shadow_ratio);
        exit(-1);
    }
    fclose(fp);
    ls_app_size = ls_world_size * shadow_ratio / (1 + 2 * shadow_ratio);
    num_of_sc = ls_world_size - 2 * ls_app_size;
    if(ls_world_rank == 0){
        int verify_num;
        if(ls_app_size % shadow_ratio == 0)
            verify_num = ls_app_size / shadow_ratio;
        else
            verify_num = ls_app_size / shadow_ratio + 1;
        if(verify_num == num_of_sc){
            printf("This is using lsMPI, the world size is %d, app size is %d\n", ls_world_size, ls_app_size);
        }
        else{
            printf("Wrong configuration! World size is %d, app size is %d, shadow ratio is %d\n", ls_world_size, ls_app_size, shadow_ratio);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
}

void ls_init(){
    int i;
    int temp;

    PMPI_Comm_dup(MPI_COMM_WORLD, &ls_cntr_world_comm);
//    PMPI_Comm_dup(MPI_COMM_WORLD, &ls_failure_world_comm);
    
    read_config();
    leap_flag = 0;
    ls_app_rank = ls_world_rank % ls_app_size;
    ls_my_coordinator = 2 * ls_app_size + ls_app_rank / shadow_ratio;
    ls_data_msg_count = 0;
    ls_cntr_msg_count = 0;
    ls_recv_counter = 0;
    ls_leap_recv_counter = -1;
    ls_my_category = ls_world_rank / ls_app_size;
    if(ls_my_category == 2){
        PMPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, ls_world_rank, &ls_data_world_comm);
    }
    else{
        PMPI_Comm_split(MPI_COMM_WORLD, ls_my_category, ls_world_rank, &ls_data_world_comm);
    }
    if(ls_my_category == 0){
        install_signal_handlers(1);
        socket_connect();
        launch_monitor_thread(1);
#ifdef DEBUG
        printf("[%d:%d]: I am a main, my pid is %d, my sc is %d\n", ls_world_rank, ls_world_size, getpid(), ls_my_coordinator);
        fflush(stdout);
#endif
    }
    else if(ls_my_category == 1){
        //init_groups();
        install_signal_handlers(0);
        report_pid();
        mq_init();
        socket_connect();
        launch_monitor_thread(0);
#ifndef DEBUG
        //suppress output
        //freopen("/dev/null", "w", stdout);
#endif
#ifdef DEBUG
        printf("[%d:%d]: I am a shadow, my pid is %d, my sc is %d\n", ls_world_rank, ls_world_size, getpid(), ls_my_coordinator);
        fflush(stdout);
#endif
        //raise(SIGKILL);
    }
    else if(ls_my_category == 2){
        int num_of_shadows;

        if(ls_world_rank != ls_world_size - 1){
            num_of_shadows = shadow_ratio;
        }
        else{
            num_of_shadows = ls_app_size - (num_of_sc - 1) * shadow_ratio;
        }
#ifdef DEBUG
        printf("[%d:%d]: I am a shadow coordinator, my pid is %d, I have %d shadows\n", ls_world_rank, ls_world_size, getpid(), num_of_shadows);
        fflush(stdout);
#endif
        coordinate(num_of_shadows);
    }
}



void report_failure(int rank){
    PMPI_Send(&rank, 1, MPI_INT, ls_my_coordinator, 0, ls_cntr_world_comm);
#ifdef DEBUG
    printf("[%d] Notified shadow coordinator of Main %d failure\n", ls_world_rank, rank); 
    fflush(stdout);
#endif
}



//void init_groups(){
//    int triple[1][3]; 
//
//    triple[0][0] = ls_app_size;
//    triple[0][1] = ls_app_size * 2 - 1;
//    triple[0][2] = 1;
//    PMPI_Group_range_excl(ls_data_world_group, 1, triple, &ls_main_group);
//    
//    triple[0][0] = 0;
//    triple[0][1] = ls_app_size - 1;
//    triple[0][2] = 1;
//    PMPI_Group_range_excl(ls_data_world_group, 1, triple, &ls_shadow_group);
//}
