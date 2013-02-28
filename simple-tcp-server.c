#include "simple-tcp-server.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "external/c_utility/net/net.h"

static void read_socket(tcp_server_ctx_t* ctx, socketlist_t* sock);
static void accept_socket(tcp_server_ctx_t* ctx);

tcp_server_ctx_t* tcp_server_new()
{
    tcp_server_ctx_t* ctx = (tcp_server_ctx_t*) malloc(sizeof(tcp_server_ctx_t));
    ctx->data = NULL;
    ctx->port = 0;
    ctx->backlog = 5;

    ctx->listen_socket = 0;
    ctx->client_sockets = NULL;

    ctx->on_accept = NULL;
    ctx->on_read = NULL;
    ctx->data_destructor = NULL;
    ctx->client_data_destructor = NULL;
    return ctx;
}

void tcp_server_free(tcp_server_ctx_t* ctx)
{
    socketlist_free(ctx->client_sockets);
    if(ctx->data_destructor != NULL){
        ctx->data_destructor(ctx->data);
    }
    free(ctx);
}

void tcp_server_close_all(tcp_server_ctx_t* ctx)
{
    if(ctx->listen_socket != 0){
        close(ctx->listen_socket);
        ctx->listen_socket = 0;
    }

    socketlist_t* cursor = ctx->client_sockets;
    while(cursor != NULL){
        if(cursor->socket != 0){
            close(cursor->socket);
        }
        cursor = cursor->next;
    }
    socketlist_free(ctx->client_sockets);
    ctx->client_sockets = NULL;
}

void tcp_server_delete_can_close_sockets(tcp_server_ctx_t* ctx)
{
    ctx->client_sockets = socketlist_delete_can_close_sockets(ctx->client_sockets);
}

void tcp_server_listen(tcp_server_ctx_t* ctx)
{
    net_result_t res;
    net_listen(&ctx->listen_socket, ctx->port, ctx->backlog, &res);
    if(res.code != NET_RESULT_OK){
        exit(EXIT_FAILURE);
    }
}

int tcp_server_fd_set(tcp_server_ctx_t* ctx, fd_set *readfds)
{
    int max;
    FD_SET(ctx->listen_socket, readfds);
    max = ctx->listen_socket;

    socketlist_t* cursor = ctx->client_sockets;
    while(cursor != NULL){
        FD_SET(cursor->socket, readfds);
        max = max > cursor->socket ? max : cursor->socket;
        cursor = cursor->next;
    }
    return max;
}

void tcp_server_fd_isset(tcp_server_ctx_t* ctx, fd_set *readfds)
{
    /* 1. check client sockets */
    socketlist_t* cursor = ctx->client_sockets;
    while(cursor != NULL){
        if(FD_ISSET(cursor->socket, readfds)){
            read_socket(ctx, cursor);
        }
        cursor = cursor->next;
    }

    /* 2. check listen socket */
    if(FD_ISSET(ctx->listen_socket, readfds)){
        accept_socket(ctx);
    }
}

static void read_socket(tcp_server_ctx_t* ctx, socketlist_t* sock)
{

#ifndef SIMPLE_TCP_SERVER_READ_BUFFER_SIZE
#define SIMPLE_TCP_SERVER_READ_BUFFER_SIZE 1024
#endif

    static uint8_t buff[SIMPLE_TCP_SERVER_READ_BUFFER_SIZE];
    ssize_t r;
    
 retry:
    r = read(sock->socket, buff, 1024);
    if(r == -1){
        int err = errno;
        perror("read fail");
        switch(err){
        case EINTR:
        case EAGAIN:
            goto retry;
            break;
        default:
            exit(EXIT_FAILURE);
        }
    }
    else if(r == 0){
        printf("SOCKET %d SET CAN CLOSE\n", sock->socket);
        sock->status = SOCKETLIST_CAN_CLOSE;
        return;
    }
    
    if(ctx->on_read != NULL){
        ctx->on_read(ctx, sock, buff, r);
    }

#undef SIMPLE_TCP_SERVER_READ_BUFFER_SIZE
}

static void accept_socket(tcp_server_ctx_t* ctx)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sd = accept(ctx->listen_socket, (struct sockaddr *)&client_addr, &client_len);
    if(client_sd < 0){
        perror("accept error");
        exit(EXIT_FAILURE);
    }

    /* SET NONBLOCK */
    int flags;
    flags = fcntl(client_sd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        exit(EXIT_FAILURE);
    }
    if (fcntl(client_sd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        exit(EXIT_FAILURE);
    }

    if(ctx->on_accept == NULL){
        ctx->client_sockets = socketlist_append(ctx->client_sockets, client_sd, ctx->client_data_destructor);
    }
    else{
        if(ctx->on_accept(ctx, &client_addr)){
            ctx->client_sockets = socketlist_append(ctx->client_sockets, client_sd, ctx->client_data_destructor);
        }
        else{
        reclose:
            if(close(client_sd) != 0){
                int err = errno;
                perror("close failed");
                switch(err){
                case EINTR:    /* retry */
                    goto reclose;
                    break;
                default:
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}
