#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "simple-tcp-server.h"
#include "external/c_utility/collections/bufferlist.h"

static void client_data_destructor(void* data)
{
    bufferlist_t* bufferlist = (bufferlist_t*) data;
    ssize_t len = bufferlist_len(bufferlist);
    printf("len = %zd\n", len);
    char* s = (char*) bufferlist_concat(bufferlist);
    printf("data = %s\n", s);
    bufferlist_concat_free((uint8_t*)s);
    bufferlist_free(bufferlist);
}

static bool on_accept(struct tcp_server_ctx_t* this, struct sockaddr_in* client_addr)
{
    puts("ON ACCEPT");
    return true;
}

static void on_read(struct tcp_server_ctx_t* this, socketlist_t* client_socket, uint8_t* buff, ssize_t len)
{
    puts("ON READ");
    client_socket->data = bufferlist_append(client_socket->data, buff, len);
    bufferlist_t* current = client_socket->data;
    while(current != NULL){
        printf("len = %zd\n", current->len);
        current = current->next;
    }
}

int main(int argc, char** argv)
{
    tcp_server_ctx_t* ctx = tcp_server_new();
    ctx->port = 8080;
    ctx->backlog = 5;
    ctx->on_accept = on_accept;
    ctx->on_read = on_read;
    ctx->client_data_destructor = client_data_destructor;
    
    tcp_server_listen(ctx);
    fd_set readfds;
    int max_fd;
    for(;;){
        FD_ZERO(&readfds);
        max_fd = tcp_server_fd_set(ctx, &readfds);
        if((select(max_fd + 1, &readfds, NULL, NULL, NULL)) == -1){
            perror("select fail");
            exit(EXIT_FAILURE);
        }
        tcp_server_fd_isset(ctx, &readfds);
        tcp_server_delete_can_close_sockets(ctx);
    }
    return 0;
}
