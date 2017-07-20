#ifndef __MONITOR_THREAD_H__
#define __MONITOR_THREAD_H__


/*monitor thread routines*/
//void* monitor_thread_main(void* arg);
//void* monitor_thread_shadow(void* arg);
void launch_monitor_thread(int is_main);
void free_thread_resources();

extern pthread_t ls_monitor_thread;
#endif
