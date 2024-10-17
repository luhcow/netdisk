#include "http.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "pool.h"

struct http_t_* http;
int startup(unsigned short int* port);
void* worker(void*) {
    int client = http->pool->queue->ops->pop(http->pool->queue);
}
int handle_func(const char* pattern, int (*handler)(int, request_t));
int listen_and_serve(const char* addr, int threads_num) {
    http = malloc(sizeof(struct http_t_));
    if (addr[0] == ':') {
        http->server.port = atoi(addr + 1);
        http->server.server_sock = startup(http->server.port);
    } else {
        http->server.interface =
                strncpy(malloc((strstr(addr, ':') - addr + 1) * sizeof(char)),
                        addr, (strstr(addr, ':') - addr) * sizeof(char));
        http->server.port = atoi(strstr(addr, ':') + 1);
        http->server.server_sock = startup(http->server.port);
    }
    fprintf(stderr, "httpd running on port %d\n", http->server.port);

    struct pool_t* pool = pool_create(worker, true, 2);
    while (1) {
        // 套接字收到客户端连接请求
        int client_sock = accept(http->server.server_sock, NULL, NULL);
        if (client_sock == -1)
            perror("accept");
        http->pool->queue->ops->push(&pool, client_sock);
    }
}

int startup(unsigned short int* port) {
    int httpd = 0;
    struct sockaddr_in name;

    /*建立 socket */
    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(httpd, (struct sockaddr*)&name, sizeof(name)) < 0)
        error_die("bind");
    /*如果当前指定端口是 0，则动态随机分配一个端口*/
    if (*port == 0) {
        int namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr*)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    /*开始监听*/
    if (listen(httpd, 5) < 0)
        error_die("listen");
    /*返回 socket id */
    return (httpd);
}