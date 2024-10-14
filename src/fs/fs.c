#include <pthread.h>
#include <stdlib.h>

static void routine(void) {
    rabbitmq_endwork();
    mysql_endwork();
    return;
}

int fs_run_(void) {
    rabbitmq_beginwork();
    mysql_beginwork();
    fs_work();
    pthread_cleanup_push(routine, NULL);
    rabbitmq_endwork();
    mysql_endwork();
    pthread_exit(NULL);
    return NULL;
    pthread_cleanup_pop(NULL);
}

void* fs_run(void* arg) {
    return fs_run_();
}