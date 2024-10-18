#ifndef ND_WD_API_GATEWAY
#define ND_WD_API_GATEWAY

#include <stdio.h>

#include "public/http.h"

int api_gateway_prework(void);
int api_gateway_endwork(void);
int startup(unsigned short int*);

int fs_handler(int client, request_t request);
int auth_handler(int client, request_t request);
static int accept_request(int);
static void bad_request(int);
static char* bad_json(const char*);
static void error_die(const char*);
static int remote_procedure_call(request_t* request);
static int get_line(int, char*, int);
static void headers(int, const char*, FILE* resource);
static void headers_204(int, const char*);
static void headers_json(int client, long len);
static void not_found(int);
static void unimplemented(int);

#endif