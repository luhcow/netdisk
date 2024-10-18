#ifndef ND_WD_API_POOL
#define ND_WD_API_POOL

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "blockq.h"

static struct pool_t {
    int thread_num;
    pthread_t *threads;
    void *(*worker)(void *);
    int (*build)(struct pool_t *pool);
    void (*cancel)(struct pool_t *pool);
    BlockQ *queue;
};
struct pool_t *pool_create(void *(*__start_routine)(void *), bool block,
                           int num);
#endif