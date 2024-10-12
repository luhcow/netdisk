#ifndef __shs_BLOCKQ_H
#define __shs_BLOCKQ_H

#include <pthread.h>
#include <stdbool.h>
#define N 1024

typedef struct BlockQ_t_ {
    int elements[N];
    int front;
    int rear;
    int size;
    pthread_mutex_t* mutex;
    pthread_cond_t* not_empty;
    pthread_cond_t* not_full;
    struct queue_ops_t_* ops;
} BlockQ;

struct queue_ops_t_ {
    int (*push)(BlockQ* q, int val);
    int (*pop)(BlockQ* q);
    int (*peek)(BlockQ* q);
    void (*destroy)(BlockQ* q);
    bool (*empty)(BlockQ* q);
    bool (*full)(BlockQ* q);
};

BlockQ* blockq_create(BlockQ* blockq);
void blockq_destroy(BlockQ* q);

bool blockq_empty(BlockQ* q);
bool blockq_full(BlockQ* q);
int blockq_push(BlockQ* q, int val);
int blockq_pop(BlockQ* q);
int blockq_peek(BlockQ* q);

#endif
