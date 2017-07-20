#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "mpi.h"
#include "mpi_override.h"
#include "msg_queue.h"
#include "shadow_leap.h"

/*global variables*/
msg_queue *shared_mq = NULL;
pthread_mutex_t mq_mutex;
msg_buf *shared_mb = NULL;



int mq_init(){
    shared_mq = (msg_queue *)malloc(sizeof(msg_queue));
    shared_mq->buffer = (msg_packet *)malloc(MSG_QUEUE_INIT_CAP * sizeof(msg_packet));
    if(shared_mq->buffer == NULL){
        printf("Error in malloc for shared mq buffer!\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    shared_mq->buffer_end = shared_mq->buffer + MSG_QUEUE_INIT_CAP;
    shared_mq->capacity = MSG_QUEUE_INIT_CAP;
    shared_mq->count = 0;
    shared_mq->head = shared_mq->buffer;
    shared_mq->tail = shared_mq->buffer;   
    pthread_mutex_init(&mq_mutex, NULL);

    shared_mb = (msg_buf *)malloc(sizeof(msg_buf));
    shared_mb->buffer = (char *)malloc(MSG_DATA_BUF_SIZE);
    if(shared_mb->buffer == NULL){
        printf("Error in malloc for shared mb buffer!\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    shared_mb->capacity = MSG_DATA_BUF_SIZE;
    shared_mb->count = 0;
    shared_mb->head = 0;
    shared_mb->tail = 0;
#ifdef DEBUG
    printf("[%d] Initialized msg queue, buffer capacity is %d\n", 
            ls_world_rank, shared_mb->capacity);
    fflush(stdout);
#endif

    return 0;
} 

int mq_clear(int count){
    pthread_mutex_lock(&mq_mutex);
    if(shared_mq->count == count){
        shared_mq->count = 0;
        shared_mq->head = shared_mq->buffer;
        shared_mq->tail = shared_mq->buffer;   
        shared_mb->count = 0;
        shared_mb->head = 0;
        shared_mb->tail = 0;
        printf("[%d] Buffer is cleared and become empty\n", ls_world_rank);
        fflush(stdout);
    }
    else if(shared_mq->count > count){
        int i;

        for(i = 0; i < count; i++){
            int length = shared_mq->head->length;

            shared_mq->head++;
            if(shared_mq->head == shared_mq->buffer_end){
                shared_mq->head = shared_mq->buffer;
            }
            shared_mq->count--;
            shared_mb->count -= length;
        }
        shared_mb->head = shared_mq->head->data;
        printf("[%d] Buffer is cleared and %d messages left\n", ls_world_rank, 
                shared_mq->count);
        fflush(stdout);
    }
 
    pthread_mutex_unlock(&mq_mutex);
    return 0;
}

int mq_free(){
    /*need to assure that all msg data are freed*/
    free(shared_mq->buffer);
    shared_mq->buffer = NULL;
    shared_mq->buffer_end = NULL;
    shared_mq->head = NULL;
    shared_mq->tail = NULL;
    free(shared_mq);
    pthread_mutex_destroy(&mq_mutex);
#ifdef DEBUG
    printf("[%d] freed msg queue\n", ls_world_rank);
    fflush(stdout);
#endif

    return 0;
}


int mq_push(int src, int tag, int length){
/* for the purpose of reducing critical section, disabled dynamically resizing */
    pthread_mutex_lock(&mq_mutex);
    if(shared_mq->count == shared_mq->capacity){
        //allocate more space for msg queue
        msg_packet *temp = (msg_packet *)malloc(shared_mq->capacity * 2 * sizeof(msg_packet));
        msg_packet *p1 = shared_mq->head, *p2 = temp;
        int i = 0;

        for(i = 0; i < shared_mq->count; i++){
            p2->src = p1->src;
            p2->tag = p1->tag;
            p2->length = p1->length;
            p2->data = p1->data;
            p2++;
            p1++;
            if(p1 == shared_mq->buffer_end){
                p1 = shared_mq->buffer;
            }
        }
        free(shared_mq->buffer);
        shared_mq->buffer = temp;
        shared_mq->capacity *= 2;
        shared_mq->buffer_end = temp + shared_mq->capacity;
        shared_mq->head = temp;
        shared_mq->tail = temp + shared_mq->count;
#ifdef DEBUG
        printf("[%d] Re-allocated for msg queue, current capacity is %d, count is %d\n", ls_world_rank, 
                shared_mq->capacity, shared_mq->count);
        fflush(stdout);
#endif
    }
//    if(shared_mq->count == shared_mq->capacity){
//        printf("[%d] mq_push, shared message queue is full! head index = %d, tail index = %d\n", ls_world_rank, shared_mq->head - shared_mq->buffer, shared_mq->tail - shared_mq->buffer);
//        MPI_Abort(MPI_COMM_WORLD, -1);
//    }
    shared_mq->tail->src = src;
    shared_mq->tail->tag = tag;
    shared_mq->tail->length = length;
    /*manage msg data*/
    shared_mq->tail->data = shared_mb->tail;
    shared_mb->tail = (shared_mb->tail + length) % shared_mb->capacity ;
    /*check threashold for leaping*/
//    if(!mb_request(length)){
//        printf("[%d] MB is full!\n", ls_world_rank);
//        /*force a leaping*/
//        MPI_Abort(MPI_COMM_WORLD, -1);
//    }
//    else{
//        shared_mq->tail->data = mb_write(length, data);
//    }

    shared_mq->tail++;
    if(shared_mq->tail == shared_mq->buffer_end){
        shared_mq->tail = shared_mq->buffer;
    }
//    pthread_mutex_lock(&mq_mutex);
    shared_mb->count += length;
    shared_mq->count++;
    pthread_mutex_unlock(&mq_mutex);
    //printf("[%d] Put a msg into shared msg queue, MB count = %d\n", ls_world_rank, shared_mb->count);
#ifdef DEBUG
        printf("[%d] Put a msg into shared msg queue, count = %d\n", ls_world_rank, shared_mq->count);
        fflush(stdout);
#endif

    return 0;
} 

int mq_pop(int *src, int *tag, int *length, void *data){
    /*busy waiting until there is a mag*/
    while(shared_mq->count == 0)
        ;
    //pthread_mutex_lock(&mq_mutex);
    *src = shared_mq->head->src;
    *tag = shared_mq->head->tag;
    *length = shared_mq->head->length;
    /*copy data and adjust data pointer*/
    mb_read(data);    
    shared_mq->head++;
    if(shared_mq->head == shared_mq->buffer_end){
        shared_mq->head = shared_mq->buffer;
    }
    pthread_mutex_lock(&mq_mutex);
    shared_mq->count--;
    pthread_mutex_unlock(&mq_mutex);
#ifdef DEBUG
        printf("[%d] Got a msg from shared msg queue, count = %d\n", ls_world_rank, shared_mq->count);
        fflush(stdout);
#endif
     ls_data_msg_count++;

    return 0;
}

/*test if there is enough space in mb to host msg*/
//int mb_request(int len){
//    return (shared_mb->capacity - shared_mb->count >= len);
//}
/* request space from mb to host msg
 * need to address the corner case that allocated space crosses buffer boundary
 * */
int mb_request(int len, int *first_seg_len){
    pthread_mutex_lock(&mq_mutex);
    if(shared_mb->capacity - shared_mb->count < len){
        pthread_mutex_unlock(&mq_mutex);
        return -1;
    }
    pthread_mutex_unlock(&mq_mutex);
    
    *first_seg_len = (shared_mb->capacity - shared_mb->tail >= len)? len: (shared_mb->capacity - shared_mb->tail);
    return shared_mb->tail;
}

int mb_reach_leap_threshold(double *utilization){
    int res = 0;
    int mb_count = 0;
    int global_mb_count = 0;
    int max_mb_count = 0;

    pthread_mutex_lock(&mq_mutex);
    mb_count = shared_mb->count;
    pthread_mutex_unlock(&mq_mutex);

    *utilization = (double)mb_count / shared_mb->capacity;
    if(mb_count >= (int)(shared_mb->capacity * MB_LEAP_THRESHOLD)){
        res = 1;
    }

    return res;
}

double get_buf_util(){
    double res = 0;

    pthread_mutex_lock(&mq_mutex);
    res = (double)(shared_mb->count) / shared_mb->capacity;
    pthread_mutex_unlock(&mq_mutex);

    return res;
}

//char *mb_index2addr(int index){
//    if(index < 0 || index >= shared_mb->capacity)
//        return NULL;
//    else
//        return shared_mb->buffer + index;
//}

/*assuming enough space, this function writes into
 * mb and return a pointer to the start
 */
//int mb_write(int len, void *data){
//    int res = shared_mb->tail;
//
//    if(shared_mb->capacity - shared_mb->tail >= len){
//        memcpy(&(shared_mb->buffer[res]), data, len);
//    }
//    else{
//        int len1 = shared_mb->capacity - shared_mb->tail;
//
//        memcpy(&(shared_mb->buffer[res]), data, len1);
//        memcpy(shared_mb->buffer, (char *)data + len1, len - len1);
//    }
//    shared_mb->tail = (shared_mb->tail + len) % shared_mb->capacity ;
//    shared_mb->count += len;
//    /*check threashold for leaping*/
//#ifdef DEBUG
//    printf("[%d] mb_write, wrote %d bytes into msg buffer, tail = %d, count = %d\n", 
//            ls_world_rank, len, shared_mb->tail, shared_mb->count);
//    fflush(stdout);
//#endif
//
//    return res;
//}

/*read a msg from mb*/
int mb_read(void *buf){
    int len = shared_mq->head->length;
    int data_pointer = shared_mq->head->data;

#ifdef DEBUG
    if(shared_mb->head != data_pointer){
        printf("[%d] mb_read, shared_mb_head = %d but shared_mq->head->data = %d\n", 
                ls_world_rank, shared_mb->head, data_pointer);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
#endif
    if(len <= shared_mb->capacity - data_pointer){
       memcpy(buf, &(shared_mb->buffer[data_pointer]), len);
    }
    else{
        int len1 = shared_mb->capacity - data_pointer;
        
        memcpy(buf, &(shared_mb->buffer[data_pointer]), len1);
        memcpy((char *)buf + len1, shared_mb->buffer, len - len1);
    }
 
    shared_mb->head = (shared_mb->head + len) % shared_mb->capacity;
    pthread_mutex_lock(&mq_mutex);
    shared_mb->count -= len;
    pthread_mutex_unlock(&mq_mutex);
#ifdef DEBUG
    printf("[%d] mb_read, read %d bytes, head = %d, count = %d\n",
            ls_world_rank, len, shared_mb->head, shared_mb->count);
    fflush(stdout);
#endif

    return 0;
} 







