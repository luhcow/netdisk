#define _POSIX_C_SOURCE 200809L

#include "public/http.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "public/handler.h"
#include "public/pool.h"
#include "public/urlcode.h"
#include "public/uthash.h"

// 全局结构体指针
struct http_t_* http;

// 函数声明
static int startup(unsigned short int* port);
static void* worker(void*);
static void not_found(int client);
static int accept_request(int client);
static void* accept_thread_(void);
static int parser(int client);
static int get_line(int sock, char* buf, int size);
static void http_exit_processing();

// 静态函数实现

/// @brief 启动服务器，绑定端口
/// @param port 端口号
/// @return socket 描述符，若失败则返回 -1
int startup(unsigned short int* port) {
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1) {
        perror("socket");
        return -1;
    }

    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(httpd, (struct sockaddr*)&name, sizeof(name)) < 0) {
        perror("bind");
        close(httpd);  // 资源释放
        return -1;
    }

    if (*port == 0) {
        int namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr*)&name, &namelen) ==
            -1) {
            perror("getsockname");
            close(httpd);
            return -1;
        }
        *port = ntohs(name.sin_port);
    }

    if (listen(httpd, 5) < 0) {
        perror("listen");
        close(httpd);
        return -1;
    }

    return httpd;
}

/// @brief 启动 HTTP 服务器
/// @param addr 地址
/// @param threads_num 线程数
/// @return 0 表示成功，-1 表示失败
int listen_and_serve(const char* addr, int threads_num,
                     int (*begin)(), int (*end)()) {
    atexit(http_exit_processing);
    http = malloc(sizeof(struct http_t_));
    if (http == NULL) {
        perror("malloc");
        return -1;
    }

    if (addr[0] == ':') {
        http->server.port = atoi(addr + 1);
        http->server.server_sock = startup(&http->server.port);
    } else {
        size_t interface_len = (strchr(addr, ':') - addr);
        http->server.interface = malloc(interface_len + 1);
        if (http->server.interface == NULL) {
            perror("malloc");
            free(http);  // 清理已分配内存
            return -1;
        }
        strncpy(http->server.interface, addr, interface_len);
        http->server.interface[interface_len] = '\0';
        http->server.port = atoi(strchr(addr, ':') + 1);
        http->server.server_sock = startup(&http->server.port);
    }

    if (http->server.server_sock == -1) {
        free(http->server.interface);
        free(http);
        return -1;
    }

    fprintf(stderr, "httpd running on port %d\n", http->server.port);

    http->begin = begin;
    http->end = end;

    struct pool_t* pool = pool_create(worker, true, threads_num);
    if (pool == NULL) {
        perror("pool_create");
        close(http->server.server_sock);  // 释放 socket
        free(http->server.interface);
        free(http);
        return -1;
    }

    if (pool->build(pool) < 0) {
        perror("pool_build");
        close(http->server.server_sock);  // 释放 socket
        free(http->server.interface);
        free(http);
        return -1;
    };

    pthread_create(&http->server.accept_thread, NULL, accept_thread_,
                   NULL);

    return 0;
}

/// @brief 接收每个连接并发送到 worker
/// @param  void
/// @return 不会返回
void* accept_thread_(void) {
    while (1) {
        // 套接字收到客户端连接请求
        int client_sock =
                accept(http->server.server_sock, NULL, NULL);
        if (client_sock == -1)
            perror("accept");
        http->pool->queue->ops->push(http->pool->queue, client_sock);
    }
}

/// @brief 处理 HTTP 请求
/// @param pattern URL 模式
/// @param handler 处理函数
/// @return 函数返回时必定成功
int handle_func(const char* pattern, int (*handler)(int, request_t)) {
    handler_add(http->server.handler, pattern, handler);

    return 0;
}

/// @brief 处理请求并发送 404 页面
/// @param client 客户端 socket 描述符
static void not_found(int client) {
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    if (send(client, buf, strlen(buf), MSG_NOSIGNAL) == -1) {
        perror("send");
        return;
    }

    sprintf(buf, "Server: netfile/0.1.0\r\n");
    if (send(client, buf, strlen(buf), MSG_NOSIGNAL) == -1) {
        perror("send");
        return;
    }
    sprintf(buf, "Content-Type: text/html\r\n");
    if (send(client, buf, strlen(buf), MSG_NOSIGNAL) == -1) {
        perror("send");
        return;
    }
    sprintf(buf, "\r\n");
    if (send(client, buf, strlen(buf), MSG_NOSIGNAL) == -1) {
        perror("send");
        return;
    }

    int not_found_html = open("./html/404.html", O_RDONLY);
    if (not_found_html <= 0) {
        perror("fopen");
        return;
    }

    struct stat st;
    fstat(not_found_html, &st);
    sendfile(client, not_found_html, 0, st.st_size);

    close(not_found_html);
}

/// @brief 接收客户端请求并处理
/// @param client 客户端 socket 描述符
/// @return 请求处理结果，若出错则返回 -1
static int accept_request(int client) {
    char buf[1024], method[255], url[255], path[512];
    char* query_string = NULL;

    int numchars = get_line(client, buf, sizeof(buf));
    if (numchars == -1) {
        perror("get_line");
        close(client);
        return -1;
    }

    size_t i = 0, j = 0;

    // 解析 HTTP 方法
    while (!isspace(buf[j]) && (i < sizeof(method) - 1)) {
        method[i++] = buf[j++];
    }
    method[i] = '\0';
    http->request.method = strdup(method);
    if (http->request.method == NULL) {
        perror("strdup");
        close(client);
        return -1;
    }

    // 解析 URL
    i = 0;
    while (isspace(buf[j]) && (j < sizeof(buf))) j++;
    while (!isspace(buf[j]) && (i < sizeof(url) - 1) &&
           (j < sizeof(buf))) {
        url[i++] = buf[j++];
    }
    url[i] = '\0';
    urldecode(url);

    http->request.uri = strdup(url);
    if (http->request.uri == NULL) {
        perror("strdup");
        free(http->request.method);
        close(client);
        return -1;
    }

    if (strchr(url + 1, '/') != NULL) {
        http->request.uri_residue = strdup(strchr(url + 1, '/'));
        if (http->request.uri_residue == NULL) {
            perror("strdup");
            free(http->request.method);
            close(client);
            return -1;
        }
    } else {
        http->request.uri_residue = NULL;
    }

    http->request.url = NULL;

    if (strcasecmp(method, "GET") == 0) {
        query_string = strchr(url, '?');
        if (query_string) {
            *query_string++ = '\0';
        }
    }
    http->request.query = query_string ? strdup(query_string) : NULL;
    if (query_string && http->request.query == NULL) {
        perror("strdup");
        free(http->request.method);
        free(http->request.uri);
        close(client);
        return -1;
    }

    if (parser(client) == -1) {
        free(http->request.method);
        free(http->request.uri);
        free(http->request.query);
        close(client);
        return -1;
    }

    void* (*handler)(void*) = handler_find(http->server.handler, url);
    if (handler == NULL) {
        not_found(client);
        close(client);
        return -1;
    } else {
        return ((int (*)(int, request_t))handler)(client,
                                                  http->request);
    }
}

/// @brief 解析 HTTP 请求头和主体
/// @param client 客户端 socket 描述符
/// @return 解析结果，-1 表示失败
static int parser(int client) {
    http->request.heads = NULL;  // 初始化为 NULL，避免未定义行为
    char temp[1024];

    for (int num = get_line(client, temp, sizeof(temp));
         num > 0 && strcmp("\n", temp) != 0;
         num = get_line(client, temp, sizeof(temp))) {
        char* tch = strstr(temp, ": ");
        if (tch == NULL)
            continue;

        char* key = strndup(temp, tch - temp);
        char* value = strndup(
                tch + 2, strlen(tch + 2) - 1);  // 去除末尾的换行符
        if (key == NULL || value == NULL) {
            perror("strndup");
            return -1;
        }

        struct str_map* t = malloc(sizeof(struct str_map));
        if (t == NULL) {
            perror("malloc");
            free(key);
            free(value);
            return -1;
        }
        t->key = key;
        t->value = value;
        HASH_ADD_STR(http->request.heads, key,
                     t);  // 使用 uthash 添加键值对

        if (strcasecmp(key, "content_length") == 0) {
            http->request.content_length = atoi(value);
        }
    }

    http->request.body = malloc(http->request.content_length);
    recv(client, http->request.body, http->request.content_length,
         MSG_WAITALL);

    return 0;
}

/// @brief 读取客户端一行数据
/// @param sock 套接字描述符
/// @param buf 缓冲区
/// @param size 缓冲区大小
/// @return 读取的字节数
static int get_line(int sock, char* buf, int size) {
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n')) {
        n = recv(sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        } else {
            c = '\n';
        }
    }
    buf[i] = '\0';
    return (i);
}

/// @brief 线程工作函数
/// @return NULL
void* worker(void*) {
    if (http->begin != NULL) {
        http->begin();
    }
    for (;;) {
        int client = http->pool->queue->ops->pop(http->pool->queue);
        if (client == -1) {
            break;
        }
        if (accept_request(client) == -1) {
            fprintf(stderr, "Error handling request\n");
        }
        close(client);
    }
    if(http->end!=NULL) {
        http->end();
    }
    return NULL;
}

void http_exit_processing() {
    pthread_cancel(http->server.accept_thread);
    pthread_join(http->server.accept_thread,NULL);
    http->pool->cancel(http->pool);
    close(http->server.server_sock);
}
