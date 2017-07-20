#ifndef _SHADOW_LEAP_
#define _SHADOW_LEAP_
#include <time.h>
#include "mpi.h"

typedef struct state_data_struct{
    void *addr;
    int count;
    MPI_Datatype dt;
    struct state_data_struct *next;
} state_data;

//extern int leap_point_sent;

//void (*leap_func_loop)(int, int);
#ifdef __cplusplus
extern "C"{
#endif 
void shadow_leap();
void ls_reset_recv_counter();
void ls_inject_failure(int rank, int iter);
void leap_register_state(void *addr, int count, MPI_Datatype dt);
void force_leaping();
int ls_sync_main_shadow();
#ifdef __cplusplus
}
#endif
#define REBOOT_TIME 0 //time to reboot after a failure
#define BUF_CHECK_INTERVAL 10 //interval for checking buffer utilization for forced leaping
#define COORDINATED_LEAPING
#endif
