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
typedef struct {
    char* bytes;
    long len;
} http_content_t;

typedef struct {
    char* content_type;
    int content_type_len;
    char* authorization;
    int authorization_len;
    char* file_path;
    int file_path_len;
    http_content_t content;
} http_request_t;

void accept_request(int);

void bad_request(int);

char* bad_json(const char*);

void cat(int, FILE*);

// void cannot_execute(int);

void error_die(const char*);

int parser(int, const char*, http_request_t*);

http_content_t remote_procedure_call(const char*, http_request_t*);

int get_line(int, char*, int);

void headers(int, const char*, FILE* resource);

void headers_204(int, const char*);

void not_found(int);

// void serve_file(int, const char*);

int startup(u_short*);

void unimplemented(int);

#endif