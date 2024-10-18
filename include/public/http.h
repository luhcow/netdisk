#ifndef ND_WD_HTTP
#define ND_WD_HTTP

#include <pthread.h>

#include "public/handler.h"
#include "public/uthash.h"

struct str_map {
    char* key;
    char* value;
    UT_hash_handle hh;
};
typedef struct request_t_ {
    struct str_map* heads;

    char* url;
    char* uri;
    char* uri_residue;
    char* method;
    char* query;
    char* body;
    unsigned long content_length;
} request_t;

typedef struct server_t_ {
    char* interface;
    unsigned short int port;
    int server_sock;
    pthread_t accept_thread;
    struct handler_map* handler;
} server_t;

struct http_t_ {
    request_t request;
    server_t server;
    struct pool_t* pool;
    int (*begin)();
    int (*end)();
};

int handle_func(const char* pattern, int (*handler)(int, request_t));
int listen_and_serve(const char* addr, int threads_num,
                     int (*begin)(), int (*end)());

#endif