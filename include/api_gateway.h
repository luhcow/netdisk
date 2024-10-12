#ifndef shs_api_gateway
#define shs_api_gateway

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "authorization.h"
#include "rpc_sending.h"
#include "urlcode.h"

typedef struct http_content_t_ {
    char* bytes;
    long len;
} http_content_t;

typedef struct http_request_t_ {
    char* content_type;
    int content_type_len;
    char* authorization;
    int authorization_len;
    char* file_path;
    int file_path_len;
    http_content_t content;
} http_request_t;

int api_gateway_prework(void);
int api_gateway_work(int);
int api_gateway_endwork(void);
int startup(unsigned short int*);

static int accept_request(int);
static void bad_request(int);
static char* bad_json(const char*);
static void cat(int, FILE*);
static void error_die(const char*);
static int parser(int, const char*, http_request_t*);
static http_content_t remote_procedure_call(const char*,
                                            const http_request_t*);
static int get_line(int, char*, int);
static void headers(int, const char*, FILE* resource);
static void headers_204(int, const char*);
static void headers_json(int client, long len);
static void not_found(int);
static void unimplemented(int);

#endif