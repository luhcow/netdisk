#ifndef ND_WD_API_POOL
#define ND_WD_API_POOL

#include <stdio.h>
#include <unistd.h>

#include "blockq.h"

void* gateway_pool_build(void* pool_void);
static void* consumer(void* task);
#endif