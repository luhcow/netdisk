#include <pthread.h>
#include <rabbitmq-c/amqp.h>

#include "fs_handler.h"
#include "handler.h"
#include "rabbitmq_p.h"

struct handler_map fs_handler;

static int fs_work(void) {
    return rabbit_consumer(&fs_handler);
}

static void thread_cleaning(void) {
    rabbitmq_endwork();
    mysql_endwork();

    return;
}

static int fs_run_(void) {
    fs_hanlder_init(&fs_handler);
    // TODO rabbit end 等待重写
    listenbegin();
    mysql_beginwork();

    fs_work();

    pthread_cleanup_push(thread_cleaning, NULL);

    rabbitmq_endwork();
    mysql_endwork();

    pthread_exit(NULL);
    return NULL;
    pthread_cleanup_pop(NULL);
}

void *fs_run(void *arg) {
    return fs_run_();
}
