#ifndef SIMPLE_TCP_SERVER_H
#define SIMPLE_TCP_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/select.h>

#include "external/c_utility/collections/socketlist.h"

typedef struct tcp_server_ctx_t{
    void* data;
    // settings
    uint16_t port;
    int backlog;
    // listen
    int listen_socket;
    socketlist_t* client_sockets;
    
    // callback
    bool(*on_accept)(struct tcp_server_ctx_t* this, struct sockaddr_in* client_addr);
    void(*on_read)(struct tcp_server_ctx_t* this, socketlist_t* client_socket, uint8_t* buff, ssize_t len);
    void(*data_destructor)(void* data);
    void(*client_data_destructor)(void* data);
} tcp_server_ctx_t;


tcp_server_ctx_t* tcp_server_new();
void tcp_server_free(tcp_server_ctx_t* ctx);
void tcp_server_close_all(tcp_server_ctx_t* ctx);
void tcp_server_delete_can_close_sockets(tcp_server_ctx_t* ctx);

void tcp_server_listen(tcp_server_ctx_t* ctx);
int tcp_server_fd_set(tcp_server_ctx_t* ctx, fd_set *readfds);
void tcp_server_fd_isset(tcp_server_ctx_t* ctx, fd_set *readfds);

#endif
