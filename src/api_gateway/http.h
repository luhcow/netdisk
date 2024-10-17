#ifndef ND_WD_HTTP
#define ND_WD_HTTP

typedef struct request_t_ {
    struct str_str_map* heads;
    struct handler_map* handler;
    char* url;
    char* uri;
    char* uri_residue;
    char* method;
} request_t;

typedef struct server_t_ {
    char* interface;
    char* port;
    int server_sock;
    pthread_t accept_thread;
} server_t;

struct http_t_ {
    request_t request;
    server_t server;
    struct pool_t* pool;
};

int handle_func(const char* pattern, int (*handler)(int, request_t));
int listen_and_serve(const char* addr, int threads_num);

#endif