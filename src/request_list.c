#include <stdio.h>
#include <stdlib.h>
#include "request_list.h"
#include "mpi_override.h"

req_list *list_head = NULL;
int list_count = 0;


int rl_init( ){
#ifdef DEBUG
    printf("initilizing request list\n");
    fflush(stdout);
#endif
    list_head = NULL;
    list_count = 0;
    return 0;
}

//req_list * construct_list_node(MPI_Request *req, void *buf){
//    req_list *temp = (req_list *)malloc(sizeof(req_list));
//    temp->req = req;
//    temp->buf = buf;
//    temp->next = NULL;
//
//    return temp;
//}


int rl_add(MPI_Request *req, void *buf){
    req_list *node = (req_list *)malloc(sizeof(req_list)); 
    //req_list *node = construct_list_node(req, buf);

    if(node == NULL){
        printf("[%d] Failed to build a request list node\n", ls_world_rank);
        fflush(stdout);
        return -1;
    }
    node->req = req;
    node->buf = buf;
    node->next = NULL;

    list_count++;
    if(list_head == NULL){
        list_head = node;
#ifdef DEBUG
        printf("[%d] Added a list node at head, count = %d, request = %ld\n", ls_world_rank, list_count, req);
        fflush(stdout);
#endif
    }
    else{
        req_list *p = list_head;
#ifdef DEBUG
        int index = 1;
#endif
        while(p->next != NULL){
            p = p->next;
#ifdef DEBUG
            index++;
#endif
        }
        p->next = node;
#ifdef DEBUG
        printf("[%d] Added a list node at index %d, count = %d, request = %ld\n", ls_world_rank, index, list_count, req);
#endif
    }

    return 0;
}

req_list *rl_find(MPI_Request *target){
#ifdef DEBUG
    printf("[%d] Entering find_req_node()\n", ls_world_rank);
#endif
    req_list *p = list_head;
#ifdef DEBUG
    int index = 0;
#endif
    while(p != NULL){
        if(p->req == target){
        //if((long)p->req == (long)target - 32){
#ifdef DEBUG
            printf("[%d] Found entry at index %d, count = %d\n", ls_world_rank, index, list_count);
            fflush(stdout);
#endif 
            return p;
        }
#ifdef DEBUG
        else{
            printf("[%d] Not equal, request = %ld, target = %ld\n", ls_world_rank, 
                    p->req, target);
        }
#endif
        p = p->next;
#ifdef DEBUG
        index++;
#endif
    }
#ifdef DEBUG
    printf("[%d] Entry not found in request list, count = %d\n", ls_world_rank, list_count);
    fflush(stdout);
#endif

    return NULL;
}



int rl_remove(req_list *node){
    if(list_head == NULL){ //empty list
#ifdef DEBUG
        printf("[%d] List is empty, entry to remove not found, count = %d\n", ls_world_rank, list_count);
        fflush(stdout);
#endif
        return -1;
    }
    else if(node == list_head){
        list_head = list_head->next;
        //!!!!need to free main_req and shadow_req first
        free(node);
        list_count--;
#ifdef DEBUG
        printf("[%d] Removed entry at index 0, count = %d\n", ls_world_rank, list_count);
        fflush(stdout);
#endif
        return 0;
    }
//    else if(list_head->next == NULL){
//#ifdef DEBUG
//        printf("[%d] Entry to remove not found, count = %d\n", ls_world_rank, list_count);
//        fflush(stdout);
//#endif
//        return -1;
//    }
    else{
        req_list *p1 = list_head;
        req_list *p2 = p1->next;
#ifdef DEBUG
        int index = 1;
#endif
        while(p2 != NULL){
            if(p2 == node){
                p1->next = p2->next;
                free(node);
                list_count--;
#ifdef DEBUG
                printf("[%d] Removed entry at index %d, count = %d\n", ls_world_rank, index, list_count);
                fflush(stdout);
#endif
                return 0;
            }
            p1 = p2;
            p2 = p2->next;
#ifdef DEBUG
            index++;
#endif
        }
#ifdef DEBUG
        printf("[%d] Entry to remove not found, count = %d\n", ls_world_rank, list_count);
        fflush(stdout);
#endif
 
        return -1;
    }
}

    

int rl_free(){
    req_list *p = list_head;

    while(p != NULL){
        list_head = list_head->next;
        p->next = NULL;
        free(p);
        p = list_head;
    }

    return 0;
}










