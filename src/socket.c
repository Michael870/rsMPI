#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef USE_RDMA
#include <rdma/rsocket.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <errno.h>
#include <ifaddrs.h>
#include <pthread.h>
#include "mpi.h"
#include "socket.h"
#include "shared.h"
#include "mpi_override.h"
#include "msg_queue.h"
#include "monitor_thread.h"
#include "shadow_leap.h"

/*global variables*/
int sock_fd;
//char *socket_buf = NULL;
//int socket_buf_size;

#ifndef USE_TCP
struct sockaddr_in my_addr, re_addr;
#endif

/*retrieve local ip address*/
int get_my_ip(char *ip){
    struct ifaddrs *myaddrs = NULL, *ifa = NULL;
    void *in_addr = NULL;
    struct sockaddr_in *s4 = NULL; 
    
    if(getifaddrs(&myaddrs) != 0)
    {
        perror("getifaddrs");
        exit(1);
    }

    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;
        if(ifa->ifa_addr->sa_family != AF_INET)
            continue;

        s4 = (struct sockaddr_in *)ifa->ifa_addr;
        in_addr = &s4->sin_addr;

        if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, ip, INET_ADDRSTRLEN))
        {
            printf("%s: inet_ntop failed!\n", ifa->ifa_name);
        }
        else if(!strncmp(ip, "10.", 3))
        {
#ifdef DEBUG
            printf("[%d] Found IP starting with 10.: %s\n", ls_world_rank, ip);
            fflush(stdout);
#endif
            break;
        }
    }

    if(strncmp(ip, "10.", 3)){
        printf("[%d] Error in finding correct IP!\n", ls_world_rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    freeifaddrs(myaddrs);
    return 0;
}

#ifdef USE_TCP
int socket_connect(){
    int temp_fd;
    int new_fd;
    int status;
    int sin_size;
    char ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in my_addr, re_addr;
    int port_inc, port_num;
    int sock_buf_len;
//    int ssize, rsize;
//    socklen_t sock_len;

#ifdef USE_RDMA
    if((temp_fd = rsocket(PF_INET, SOCK_STREAM, 0)) == -1){
#else
    if((temp_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
#endif
        printf("[%d] Error in socket()!\n", ls_world_rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if(ls_my_category == 0){

        if(get_my_ip(ipstr) != 0){
            printf("[%d] Error in finding correct IP!\n", ls_world_rank);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = inet_addr(ipstr);
        memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
        for(port_inc = 0; port_inc < BIND_TIMES; port_inc++){
            port_num = PORT_NUM + ls_app_rank + port_inc * 100;
            my_addr.sin_port = htons((short)port_num);
#ifdef USE_RDMA
            if(rbind(temp_fd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1){
#else
            if(bind(temp_fd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1){
#endif
                if(port_inc + 1 >= BIND_TIMES){
#ifdef USE_RDMA
                    rclose(sock_fd);
#else
                    rclose(sock_fd);
#endif
                    printf("[%d] Error in bind(), giving up and aborting!\n", ls_world_rank);
                    MPI_Abort(MPI_COMM_WORLD, -1);
                }
                else{
                    printf("[%d] Error in bind()!\n", ls_world_rank);
                }
            }
            else{
                break;
            }
        }
#ifdef USE_RDMA
        if(rlisten(temp_fd, 5) == -1){
#else
        if(listen(temp_fd, 5) == -1){
#endif
            printf("[%d] Error in listen()!\n", ls_world_rank);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        sin_size = sizeof re_addr;
        /*send my ip and port to shadow*/
        PMPI_Send(ipstr, strlen(ipstr) + 1, MPI_CHAR, ls_world_rank + ls_app_size, SHADOW_IP_TAG, ls_cntr_world_comm);
        PMPI_Send(&port_num, 1, MPI_INT, ls_world_rank + ls_app_size, SHADOW_IP_TAG, ls_cntr_world_comm);
#ifdef USE_RDMA
        sock_fd = raccept(temp_fd, (struct sockaddr *)&re_addr, &sin_size);
#else
        sock_fd = accept(temp_fd, (struct sockaddr *)&re_addr, &sin_size);
#endif
        if(new_fd == -1){
            printf("[%d] Error in accept()!\n", ls_world_rank);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
//        inet_ntop(AF_INET, &(re_addr.sin_addr), s, sizeof s);
#ifdef DEBUG
        printf("[%d]: got connection from my shadow\n", ls_world_rank);
        fflush(stdout);
#endif
#ifdef USE_RDMA
        rclose(temp_fd);
#else
        close(temp_fd);
#endif
        /*main only do send(), so make socket send buffer as large as possible*/
        sock_buf_len = 2 << 22;
#ifdef USE_RDMA
        if(rsetsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &sock_buf_len, sizeof(int)) == -1){
#else
        if(setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &sock_buf_len, sizeof(int)) == -1){
#endif
            printf("[%d] setsockopt: %s\n", ls_world_rank, strerror(errno));
        }
        sock_buf_len = 2 << 10;
#ifdef USE_RDMA
        if(rsetsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &sock_buf_len, sizeof(int)) == -1){
#else
        if(setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &sock_buf_len, sizeof(int)) == -1){
#endif
            printf("[%d] setsockopt: %s\n", ls_world_rank, strerror(errno));
        }
    }
    else{
        /*receive ip and port of main*/
        PMPI_Recv(ipstr, INET_ADDRSTRLEN, MPI_CHAR, ls_app_rank, SHADOW_IP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        PMPI_Recv(&port_num, 1, MPI_INT, ls_app_rank, SHADOW_IP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        sock_fd = temp_fd;
        re_addr.sin_family = AF_INET;
        re_addr.sin_port = htons((short)port_num);
        re_addr.sin_addr.s_addr = inet_addr(ipstr);
        memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
#ifdef USE_RDMA
        if(rconnect(sock_fd, (struct sockaddr *)&re_addr, sizeof re_addr) == -1){
#else
        if(connect(sock_fd, (struct sockaddr *)&re_addr, sizeof re_addr) == -1){
#endif
#ifdef USE_RDMA
            rclose(sock_fd);
#else
            close(sock_fd);
#endif
            perror("client: connect");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
#ifdef DEBUG
        printf("[%d] connected to main\n", ls_world_rank);
        fflush(stdout);
#endif
        /*shadow only do recv(), so make socket recv buffer as large as possible*/
        sock_buf_len = 2 << 10;
#ifdef USE_RDMA
        if(rsetsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &sock_buf_len, sizeof(int)) == -1){
#else
        if(setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &sock_buf_len, sizeof(int)) == -1){
#endif
            printf("[%d] setsockopt: %s\n", ls_world_rank, strerror(errno));
        }
        sock_buf_len = 2 << 22;
#ifdef USE_RDMA
        if(rsetsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &sock_buf_len, sizeof(int)) == -1){
#else
        if(setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &sock_buf_len, sizeof(int)) == -1){
#endif
            printf("[%d] setsockopt: %s\n", ls_world_rank, strerror(errno));
        }
    }
//    sock_len = sizeof(int);
//    getsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &ssize, &sock_len);
//    sock_len = sizeof(int);
//    getsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &rsize, &sock_len);
//    printf("[%d] ssize = %d, rsize = %d\n", ls_world_rank, ssize, rsize); 

    //socket_buf = (char *)malloc(SOCKET_BUF_INIT_SIZE);
    //socket_buf_size = SOCKET_BUF_INIT_SIZE;

    return 0;
}

#else

int socket_connect(){
    int temp_fd;
    int new_fd;
    int status;
    int sin_size;
    char my_ipstr[INET_ADDRSTRLEN], re_ipstr[INET_ADDRSTRLEN];
    int port_inc, my_port_num, re_port_num;


    if((temp_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1){
        printf("[%d] Error in socket()!\n", ls_world_rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if(get_my_ip(my_ipstr) != 0){
        printf("[%d] Error in finding correct IP!\n", ls_world_rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr(my_ipstr);
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
    for(port_inc = 0; port_inc < BIND_TIMES; port_inc++){
        my_port_num = PORT_NUM + ls_app_rank + port_inc * 100;
        my_addr.sin_port = htons((short)my_port_num);
        if(bind(temp_fd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1){
            if(port_inc + 1 >= BIND_TIMES){
                close(temp_fd);
                printf("[%d] Error in bind(), giving up and aborting!\n", ls_world_rank);
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
            else{
                printf("[%d] Error in bind()!\n", ls_world_rank);
            }
        }
        else{
            break;
        }
    }
    printf("[%d] My ip is %s, port is %d\n", ls_world_rank, my_ipstr, my_port_num);
    if(ls_my_category == 0){


//        if(listen(temp_fd, 5) == -1){
//            printf("[%d] Error in listen()!\n", ls_world_rank);
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
//        sin_size = sizeof re_addr;
        /*send my ip and port to shadow*/
        PMPI_Send(my_ipstr, strlen(my_ipstr) + 1, MPI_CHAR, ls_world_rank + ls_app_size, SHADOW_IP_TAG, ls_cntr_world_comm);
        PMPI_Send(&my_port_num, 1, MPI_INT, ls_world_rank + ls_app_size, SHADOW_IP_TAG, ls_cntr_world_comm);
        PMPI_Recv(re_ipstr, INET_ADDRSTRLEN, MPI_CHAR, ls_app_rank + ls_app_size, SHADOW_IP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        PMPI_Recv(&re_port_num, 1, MPI_INT, ls_app_rank + ls_app_size, SHADOW_IP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        printf("[%d] My shadow ip is %s, port is %d\n", ls_world_rank, re_ipstr, re_port_num);
        sock_fd = temp_fd;
        re_addr.sin_family = AF_INET;
        re_addr.sin_port = htons((short)re_port_num);
        re_addr.sin_addr.s_addr = inet_addr(re_ipstr);
        memset(re_addr.sin_zero, '\0', sizeof re_addr.sin_zero);
//        if(connect(sock_fd, (struct sockaddr *)&re_addr, sizeof re_addr) == -1){
//            perror("client: connect");
//            close(sock_fd);
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
////#ifdef DEBUG
//        printf("[%d] connected to shadow\n", ls_world_rank);
//        fflush(stdout);
////#endif
  
//        sock_fd = accept(temp_fd, (struct sockaddr *)&re_addr, &sin_size);
//        if(new_fd == -1){
//            printf("[%d] Error in accept()!\n", ls_world_rank);
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
//        inet_ntop(AF_INET, &(re_addr.sin_addr), s, sizeof s);
    }
    else{
        PMPI_Send(my_ipstr, strlen(my_ipstr) + 1, MPI_CHAR, ls_app_rank , SHADOW_IP_TAG, ls_cntr_world_comm);
        PMPI_Send(&my_port_num, 1, MPI_INT, ls_app_rank, SHADOW_IP_TAG, ls_cntr_world_comm);
        PMPI_Recv(re_ipstr, INET_ADDRSTRLEN, MPI_CHAR, ls_app_rank, SHADOW_IP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        PMPI_Recv(&re_port_num, 1, MPI_INT, ls_app_rank, SHADOW_IP_TAG, ls_cntr_world_comm, MPI_STATUS_IGNORE);
        printf("[%d] My main ip is %s, port is %d\n", ls_world_rank, re_ipstr, re_port_num);
        sock_fd = temp_fd;
        re_addr.sin_family = AF_INET;
        re_addr.sin_port = htons((short)re_port_num);
        re_addr.sin_addr.s_addr = inet_addr(re_ipstr);
        memset(re_addr.sin_zero, '\0', sizeof re_addr.sin_zero);
//        if(connect(sock_fd, (struct sockaddr *)&re_addr, sizeof re_addr) == -1){
//            perror("client: connect");
//            close(sock_fd);
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
////#ifdef DEBUG
//        printf("[%d] connected to main\n", ls_world_rank);
//        fflush(stdout);
////#endif
    }

    //socket_buf = (char *)malloc(SOCKET_BUF_INIT_SIZE);
    //socket_buf_size = SOCKET_BUF_INIT_SIZE;

    return 0;
}
#endif

/* main forwards received msg to shadow using this send
 * encode MPI header info into socket msg payload
 */
//int socket_send(void *buf, int len, int src, int tag){
//    int sent_bytes, remain_bytes;
//    int count;
//
//#ifdef DEBUG
//    printf("[%d] len = %d, src = %d, tag = %d\n", ls_world_rank, len, src, tag);
//    fflush(stdout);
//#endif
//   /*allocate more buffer if necessary*/
//    if(len + 3 * sizeof(int) > socket_buf_size){
//#ifdef DEBUG
//        printf("[%d] socket buf size too small, current size is %d, msg length is %d\n", 
//              ls_world_rank, socket_buf_size, len);
//        fflush(stdout);
//#endif
//        while(len + 3 * sizeof(int) > socket_buf_size){
//            socket_buf_size *= 2;
//        }
//        free(socket_buf);
//        socket_buf = (char *)malloc(socket_buf_size);
//        if(socket_buf == NULL){
//            printf("[%d] Error in malloc for socket_buf of size %d in socket_send\n", ls_world_rank, socket_buf_size);
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
//
//    }
//    memcpy((void *)socket_buf, (const void *)&len, sizeof(int));
//    memcpy((void *)(socket_buf + (int)sizeof(int)), (const void *)&src, sizeof(int));
//    memcpy((void *)(socket_buf + 2 * (int)sizeof(int)), (const void *)&tag, sizeof(int));
//    memcpy((void *)(socket_buf + 3 * (int)sizeof(int)), (const void *)buf, len);
//
//    sent_bytes = 0;
//    remain_bytes = len + 3 * sizeof(int);
//    
//    while(remain_bytes > 0){
//#ifdef USE_TCP
//#ifdef USE_RDMA
//        count = rsend(sock_fd, socket_buf + sent_bytes, remain_bytes, 0);
//#else
//        count = send(sock_fd, socket_buf + sent_bytes, remain_bytes, 0);
//#endif
//#else
//        count = sendto(sock_fd, socket_buf + sent_bytes, remain_bytes, 0, (const struct sockaddr *)&re_addr, sizeof(re_addr));
//#endif
//        if(count == -1){
//            perror("Error in socket_send()");
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
//        sent_bytes += count;
//        remain_bytes -= count;
//#ifdef DEBUG
//        printf("[%d] socket_send, sent %d bytes, %d bytes remaining\n", ls_world_rank, count, remain_bytes);
//        fflush(stdout);
//#endif
//    }
//#ifdef DEBUG
//    printf("[%d] socket_send, src = %d, tag = %d, len = %d\n", ls_world_rank, src, tag, len);
//    fflush(stdout);
//#endif 
//      
//    return 0;
//}

int socket_send(void *buf, int len, int src, int tag){
    int sent_bytes, remain_bytes;
    int count;
    int header[3];

#ifdef DEBUG
    printf("[%d] begin socket_send, len = %d, src = %d, tag = %d\n", ls_world_rank, len, src, tag);
    fflush(stdout);
#endif
    header[0] = len;
    header[1] = src;
    header[2] = tag;
    sent_bytes = 0;
    remain_bytes = 3 * sizeof(int);
    while(remain_bytes > 0){
#ifdef USE_RDMA
        count = rsend(sock_fd, (char *)header + sent_bytes, remain_bytes, 0);
#else
        count = send(sock_fd, (char *)header + sent_bytes, remain_bytes, 0);
#endif   
        if(count == -1){
            perror("Error in socket_send()");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        sent_bytes += count;
        remain_bytes -= count;
    }
    
    sent_bytes = 0;
    remain_bytes = len; 
    
    while(remain_bytes > 0){
#ifdef USE_RDMA
        count = rsend(sock_fd, (char *)buf + sent_bytes, remain_bytes, 0);
#else
        count = send(sock_fd, (char *)buf + sent_bytes, remain_bytes, 0);
#endif
        if(count == -1){
            perror("Error in socket_send()");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        sent_bytes += count;
        remain_bytes -= count;
#ifdef DEBUG
        printf("[%d] socket_send, sent %d bytes, %d bytes remaining\n", ls_world_rank, count, remain_bytes);
        fflush(stdout);
#endif
    }
#ifdef DEBUG
    printf("[%d] socket_send, src = %d, tag = %d, len = %d\n", ls_world_rank, src, tag, len);
    fflush(stdout);
#endif 
    ls_data_msg_count++;  

    return 0;
}



/*shadow receives msg from its main*/
/*didn't use parameter buf at all, need to change this along with design of shared msg queue*/
//int socket_recv(void *buf, int *len, int *src, int *tag){
//int socket_recv(){
//    int count;
//    int recv_bytes, remain_bytes;
//    int src, tag, len;
//
//    remain_bytes = 3 * sizeof(int);
//    recv_bytes = 0;
//    while(remain_bytes > 0){
//        count = recv(sock_fd, socket_buf + recv_bytes, remain_bytes, 0);
//        if(count == -1){
//            printf("[%d] Error in socket_recv() for msg header!\n", ls_world_rank);
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
//        else if(count == 0){
//#ifdef DEBUG
//            printf("[%d] Main has closed connection, now exiting\n", ls_world_rank);
//            fflush(stdout);
//#endif
//            pthread_exit(NULL);
//        }
//        remain_bytes -= count;
//        recv_bytes += count;
//    }
//    memcpy((void *)&len, (const void *)socket_buf, sizeof(int));
//    memcpy((void *)&src, (const void *)(socket_buf + (int)sizeof(int)), sizeof(int));
//    memcpy((void *)&tag, (const void *)(socket_buf + 2 * (int)sizeof(int)), sizeof(int));
//    /*allocate buffer from msg data buffer*/
//    if(len > socket_buf_size){
//#ifdef DEBUG
//        printf("[%d] socket buf size too small, current size is %d, msg length is %d\n", 
//                ls_world_rank, socket_buf_size, len);
//        fflush(stdout);
//#endif
//        while(len > socket_buf_size){
//            socket_buf_size *= 2;
//        }
//        free(socket_buf);
//        socket_buf = (char *)malloc(socket_buf_size);
//        if(socket_buf == NULL){
//            printf("[%d] Error in malloc for socket_buf of size %d in socket_recv\n", 
//                    ls_world_rank, socket_buf_size);
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
//    }
//
//    remain_bytes = len;
//    recv_bytes = 0;
//    while(remain_bytes > 0){
//        count = recv(sock_fd, socket_buf + recv_bytes, remain_bytes, 0);
//        if(count == -1){
//            perror("Error in socket_recv() for msg payload");
//            MPI_Abort(MPI_COMM_WORLD, -1);
//        }
//        remain_bytes -= count;
//        recv_bytes += count;
//    }
//#ifdef DEBUG
//    printf("[%d] socket_recv, src = %d, tag = %d, len = %d\n", ls_world_rank, src, tag, len);
//    fflush(stdout);
//#endif
//    /*to be modified*/
//    mq_push(src, tag, len, socket_buf);
//    //memcpy(buf, (const void *)socket_buf, *len);
//
//    return 0;
//}

int socket_recv(){
    int count;
    int recv_bytes, remain_bytes;
    int header[3];
    char *buf = NULL;
    int buf_index, buf_len;
    socklen_t addrlen;
    double buf_util;

    remain_bytes = 3 * sizeof(int);
    recv_bytes = 0;
    while(remain_bytes > 0){
#ifdef USE_TCP
#ifdef USE_RDMA
        count = rrecv(sock_fd, (char *)header + recv_bytes, remain_bytes, 0);
#else
        count = recv(sock_fd, (char *)header + recv_bytes, remain_bytes, 0);
#endif
#else
        count = recvfrom(sock_fd, (char *)header + recv_bytes, remain_bytes, 0, NULL, NULL);
#endif
        if(count == -1){
            perror("recvfrom for msg header\n");
            printf("[%d] Error in socket_recv() for msg header!\n", ls_world_rank);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        else if(count == 0){
#ifdef DEBUG
            printf("[%d] Main has closed connection, now exiting\n", ls_world_rank);
            fflush(stdout);
#endif
            pthread_exit(NULL);
        }
        remain_bytes -= count;
        recv_bytes += count;
    }
 #ifdef DEBUG
    printf("[%d] socket_recv, got msg header, src = %d, tag = %d, len = %d\n", 
                ls_world_rank, header[1], header[2], header[0]);
    fflush(stdout);
#endif
//    memcpy((void *)&len, (const void *)socket_buf, sizeof(int));
//    memcpy((void *)&src, (const void *)(socket_buf + (int)sizeof(int)), sizeof(int));
//    memcpy((void *)&tag, (const void *)(socket_buf + 2 * (int)sizeof(int)), sizeof(int));
//    count = recv(sock_fd, header, 3 * sizeof(int), 0);
//    if(count == -1){
//        printf("[%d] Error in socket_recv() for msg header!\n", ls_world_rank);
//        MPI_Abort(MPI_COMM_WORLD, -1);
//    }
//    else if(count == 0){
//#ifdef DEBUG
//        printf("[%d] Main has closed connection, now exiting\n", ls_world_rank);
//        fflush(stdout);
//#endif
//        pthread_exit(NULL);
//    }
//#ifdef DEBUG
//    else{
//        printf("[%d] socket_recv, got msg header, src = %d, tag = %d, len = %d\n", 
//                ls_world_rank, header[1], header[2], header[0]);
//        fflush(stdout);
//    }
//#endif
    buf_index = mb_request(header[0], &buf_len);
    if(buf_index < 0){
        printf("[%d] mb is full!\n", ls_world_rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
#ifdef DEBUG
    else{
        printf("[%d] Requested len = %d, allocated len = %d, buffer index = %d\n", 
                ls_world_rank, header[0], buf_len, buf_index);
        fflush(stdout);
    }
#endif
//    buf = mb_index2addr(buf_index);
    buf = shared_mb->buffer + buf_index;
    if(buf == NULL){
        printf("[%d] socket_recv, buffer address in mb is NULL\n", ls_world_rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    remain_bytes = buf_len;
    recv_bytes = 0;
    while(remain_bytes > 0){
#ifdef USE_TCP
#ifdef USE_RDMA
        count = rrecv(sock_fd, buf + recv_bytes, remain_bytes, 0);
#else
        count = recv(sock_fd, buf + recv_bytes, remain_bytes, 0);
#endif
#else
        count = recvfrom(sock_fd, buf + recv_bytes, remain_bytes, 0, NULL, NULL);
#endif
        if(count == -1){
            int error_code = errno;
            printf("%d %d\n", error_code, EFAULT);
            perror("Error in socket_recv() for msg payload");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        else if(count == 0){
#ifdef DEBUG
            printf("[%d] Main has closed connection, now exiting\n", ls_world_rank);
            fflush(stdout);
#endif
            pthread_exit(NULL);
        }
#ifdef DEBUG
        else{
            printf("[%d] Recv %d bytes, %d bytes remaining\n", ls_world_rank, count, remain_bytes - count);
            fflush(stdout);
        }
#endif
        remain_bytes -= count;
        recv_bytes += count;
    }
    if(buf_len < header[0]){
        //buf = mb_index2addr(0);
        buf = shared_mb->buffer;
        remain_bytes = header[0] - buf_len;
        recv_bytes = 0;
        while(remain_bytes > 0){
#ifdef USE_TCP
#ifdef USE_RDMA
            count = rrecv(sock_fd, (char *)buf + recv_bytes, remain_bytes, 0);
#else
            count = recv(sock_fd, (char *)buf + recv_bytes, remain_bytes, 0);
#endif
#else
            count = recvfrom(sock_fd, (char *)buf + recv_bytes, remain_bytes, 0, NULL, NULL);
#endif
            if(count == -1){
                int error_code = errno;
                printf("%d %d\n", error_code, EFAULT);
                perror("Error in socket_recv() for msg payload");
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
            else if(count == 0){
#ifdef DEBUG
                printf("[%d] Main has closed connection, now exiting\n", ls_world_rank);
                fflush(stdout);
#endif
                pthread_exit(NULL);
            }
#ifdef DEBUG
            else{
                printf("[%d] Recv %d bytes, %d bytes remaining\n", ls_world_rank, count, remain_bytes);
                fflush(stdout);
            }
#endif
            remain_bytes -= count;
            recv_bytes += count;
        }
    }
#ifdef DEBUG
    printf("[%d] socket_recv, src = %d, tag = %d, len = %d\n", ls_world_rank, header[1], header[2], header[0]);
    fflush(stdout);
#endif
    mq_push(header[1], header[2], header[0]);
    /*check for buffer overflow*/
    if(!leap_flag && mb_reach_leap_threshold(&buf_util)){
        shadow_force_leaping(buf_util);
    }

    return 0;
}

/*monitor_thread runs this function and listens for incoming msg from main*/
int wait_for_msg(){
    fd_set read_fds;
    fd_set master;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(sock_fd, &master);
    
    while(1){
        read_fds = master;
#ifdef USE_RDMA
        if(rselect(sock_fd + 1, &read_fds, NULL, NULL, NULL) == -1){
#else
        if(select(sock_fd + 1, &read_fds, NULL, NULL, NULL) == -1){
#endif
            printf("[%d] Error in select()!\n", ls_world_rank);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        if(FD_ISSET(sock_fd, &read_fds)){
            socket_recv();
        }
        else{
            printf("[%d] Error: sock_fd NOT in read_fds after select()\n", ls_world_rank);
            MPI_Abort(MPI_COMM_WORLD, -1);
        } 
    }
    return 0;
}

