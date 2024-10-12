// 请实现生产者消费者模型，完善下面程序：
#include "pool.h"

#include "api_gateway.h"

pthread_t pthreads[32];

static void routine(void* pool_void) {
    BlockQ* pool = (BlockQ*)pool_void;
    for (int i = 0; i < 5; i++) {
        pool->ops.push(pool, -1);
    }
    for (int i = 0; i < 5; i++) {
        pthread_join(pthreads[i], NULL);
    }
    return;
}

void* gateway_pool_build(void* pool_void) {
    BlockQ* pool = (BlockQ*)pool_void;
    blockq_create(pool);
    pthread_cleanup_push(routine, pool_void);
    for (int i = 0; i < 5; i++) {
        pthread_create(pthreads + i, NULL, consumer, pool);
    }
    while (sleep(100))
        ;
    pthread_exit(NULL);
    return NULL;
    pthread_cleanup_pop(pool_void);
}

static void* consumer(BlockQ* task) {
    api_gateway_prework();
    for (;;) {
        int num = task->ops.pop(task);
        if (num == -1) {
            printf("0x%lx get sign to exit, done\n", pthread_self());
            return;
        }
        api_gateway_work(task);
    }
    api_gateway_endwork();
    return;
}