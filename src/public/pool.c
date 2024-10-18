
#include "pool.h"

static void cancel(struct pool_t* pool) {
    fprintf(stderr, "wait %d task in queue\n", pool->queue->ops->num(pool));
    for (int i = 0; i < pool->thread_num; i++) {
        pool->queue->ops->push(pool, -1);
    }
    for (int i = 0; i < pool->thread_num; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    perror("各个线程已取消");
    return;
}

static void cancel_noblock(struct pool_t* pool) {
    fprintf(stderr, "wait %d task\n", pool->thread_num);
    for (int i = 0; i < pool->thread_num; i++) {
        pthread_cancel(pool->threads[i]);
    }
    for (int i = 0; i < pool->thread_num; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    perror("各个线程已取消");
    return;
}

int pool_build(struct pool_t* pool) {
    blockq_create(pool->queue);
    for (int i = 0; i < pool->thread_num; i++) {
        pthread_create(pool->threads + i, NULL, pool->worker, pool);
    }

    return 0;
}

int pool_build_noblock(struct pool_t* pool) {
    for (int i = 0; i < pool->thread_num; i++) {
        pthread_create(pool->threads + i, NULL, pool->worker, pool);
    }

    return 0;
}

struct pool_t* pool_create(void* (*__start_routine)(void*), bool block,
                           int num) {
    struct pool_t* pool = malloc(sizeof(struct pool_t));
    pool->thread_num = num;
    pool->threads = malloc(num * sizeof(pthread_t));
    pool->worker = __start_routine;
    if (!block) {
        pool->queue = NULL;
        pool->build = pool_build_noblock;
        pool->cancel = cancel_noblock;
    } else {
        pool->queue = NULL;
        pool->build = pool_build;
        pool->cancel = cancel;
    }

    return pool;
}