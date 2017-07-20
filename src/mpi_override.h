#ifndef __MPI_OVERRIDE_H__
#define __MPI_OVERRIDE_H__

//#define SHADOW_ACK_TAG 0
#define SHADOW_LEAP_TAG 1
#define SHADOW_SRC_TAG 2
#define SHADOW_RECV_POINT_TAG 3
#define SHADOW_RECV_COUNT_TAG 4
#define SHADOW_SYNC_TAG 5
#define SHADOW_CART_NEWRANK_TAG 6
#define SHADOW_IP_TAG 7
#define SHADOW_PROBE_TAG 8
#define SHADOW_TEST_TAG 9
#define SHADOW_WAITANY_TAG 10
#define SHADOW_WAITSOME_TAG 11
#define SHADOW_TESTALL_TAG 12
#define SHADOW_FORCE_LEAPING_TAG 13
//#define SHADOW_LEAPING 
//#define LS_MSG_SRC_BUF_LENGTH 65536 
//#define DEBUG
//#define TIME_DEBUG

extern int ls_world_size, ls_world_rank, ls_app_size, ls_app_rank;
extern MPI_Comm ls_data_world_comm, ls_cntr_world_comm;
//extern MPI_Group ls_data_world_group, ls_main_group, ls_shadow_group;
extern int ls_my_category;// 0: main, 1: shadow, 2: shadow coordinator, 3: new main
extern int num_of_sc, shadow_ratio;
extern int ls_data_msg_count, ls_cntr_msg_count;
extern int ls_recv_counter; //# of recvs in current loop
extern int ls_leap_recv_counter; //# of recvs before leaping in current loop
//extern MPI_Group ls_world_group;
extern int ls_my_coordinator;
//extern int is_killed;// 1: killed by shadow coordinator
extern int leap_flag;// 1: leap from main to shadow; 0: not to leap
//extern int ls_my_main_failure_point; // iteration count
//extern int *ls_msg_src;
//extern int ls_msg_src_index;
//extern pthread_mutex_t mt_mutex;


#endif
