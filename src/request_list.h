#ifndef __REQUEST_LIST_H__
#define __REQUEST_LIST_H__

#include "mpi.h"

typedef struct request_list{
    MPI_Request *req;
    void *buf;
    struct request_list *next;
} req_list;


int rl_init();
int rl_add(MPI_Request *req, void *buf);
req_list *rl_find(MPI_Request *target);
int rl_remove(req_list *node);
int rl_free();

#endif
