#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include "mpi.h"
#include "mpi_override.h"
#include "shared.h"
#include "monitor_thread.h"
/*
 * signal handlers for control panel
 * USR1: terminate shadow process
 * USR2: notify shadow of its main's failure
 * INT: notify shadow to leap
 * TERM: compute thread notifies monitor thread to exit
 */

struct sigaction ls_sigaction_term, ls_sigaction_fail, ls_sigaction_leap;


///*coordinator use SIGUSR2 to notify shadow of its main's failure*/
//void main_failed(int signo){
//    int temp[2];
//    printf("[%d] recv notification from SC of my main's failure\n", ls_world_rank);
//    fflush(stdout);
//
//    PMPI_Recv(temp, 2, MPI_INT, ls_app_rank, 0, ls_failure_world_comm, MPI_STATUS_IGNORE);
//    ls_my_main_failure_point = temp[0];
//    ls_leap_recv_counter = temp[1];
//    printf("[%d] recv from my main of failure point at iter %d and recv counter %d\n", ls_world_rank, ls_my_main_failure_point, ls_leap_recv_counter);
//    fflush(stdout);
//}

/*
 * terminate the helper thread at the main
 */
void terminate_helper(int signo){
#ifdef DEBUG
    printf("[%d] helper thread caught terminate signal\n", ls_world_rank);
    fflush(stdout);
#endif
    pthread_exit(NULL);
}


/*
 * exit the shadow process when main completes
 */
void terminate_shadow(int signo){
#ifdef DEBUG
    printf("[%d]: caught terminate signal\n", ls_world_rank);
    fflush(stdout);
#endif
//    is_killed = 1;
    /*terminate monitor thread first*/
    //pthread_kill(ls_monitor_thread, SIGALRM);    
    pthread_join(ls_monitor_thread, NULL);
    free_thread_resources();
    mq_free();
    free_resources();
#ifdef DEBUG
    printf("[%d]: I am calling PMPI_Finalize().\n", ls_world_rank);
    fflush(stdout);
#endif
 
    PMPI_Finalize();
#ifdef DEBUG
    printf("[%d]: end Finalize\n", ls_world_rank);
    fflush(stdout);
#endif

    exit(0);
}


/* used by the shadow coordinator to triger a shadow leaping
 * in a shadow process
 */
void leap_myself(int signo){
#ifdef DEBUG
    printf("[%d]: caught leap signal\n", ls_world_rank);
    fflush(stdout);
#endif
    ////////////set flag
    leap_flag = 1;
    PMPI_Recv(&ls_leap_recv_counter, 1, MPI_INT, ls_app_rank, SHADOW_RECV_POINT_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
    printf("[%d] recv leap point (recv counter = %d) from my main\n", ls_world_rank, ls_leap_recv_counter);
    fflush(stdout);
}

void install_signal_handlers(int is_main){
    if(is_main){
        ls_sigaction_term.sa_handler = terminate_helper;
        ls_sigaction_term.sa_flags = 0;
        sigemptyset(&ls_sigaction_term.sa_mask);
        sigaction(SIGUSR1, &ls_sigaction_term, NULL);
    }
    else{
        ls_sigaction_term.sa_handler = terminate_shadow;
        ls_sigaction_term.sa_flags = 0;
        sigemptyset(&ls_sigaction_term.sa_mask);
        sigaction(SIGUSR1, &ls_sigaction_term, NULL);
    
    //    ls_sigaction_fail.sa_handler = main_failed;
    //    ls_sigaction_fail.sa_flags = 0;
    //    sigemptyset(&ls_sigaction_fail.sa_mask);
    //    sigaction(SIGUSR2, &ls_sigaction_fail, NULL); 
    
        ls_sigaction_leap.sa_handler = leap_myself;;
        ls_sigaction_leap.sa_flags = 0;
        sigemptyset(&ls_sigaction_leap.sa_mask);
        sigaction(SIGTERM, &ls_sigaction_leap, NULL); 
    }
}










