#include "blockq.h"

BlockQ* blockq_create(BlockQ* blockq) {
    blockq->front = 0;
    blockq->size = 0;
    blockq->rear = 0;

    blockq->mutex = malloc(sizeof(pthread_mutex_t));
    blockq->not_empty = malloc(sizeof(pthread_cond_t));
    blockq->not_full = malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(blockq->mutex, NULL);
    pthread_cond_init(blockq->not_empty, NULL);
    pthread_cond_init(blockq->not_full, NULL);
    blockq->ops.destroy = blockq_destroy;
    blockq->ops.empty = blockq_empty;
    blockq->ops.full = blockq_full;
    blockq->ops.peek = blockq_peek;
    blockq->ops.pop = blockq_pop;
    blockq->ops.push = blockq_push;
    return blockq;
}
void blockq_destroy(BlockQ* q) {
    pthread_mutex_destroy(q->mutex);
    pthread_cond_destroy(q->not_empty);
    pthread_cond_destroy(q->not_full);

    free(q->mutex);
    free(q->not_empty);
    free(q->not_full);
    free(q);
    return;
}

bool blockq_empty(BlockQ* q) {
    pthread_mutex_lock(q->mutex);
    int size = q->size;
    pthread_mutex_unlock(q->mutex);
    return size == 0;
}
bool blockq_full(BlockQ* q) {
    pthread_mutex_lock(q->mutex);
    int size = q->size;
    pthread_mutex_unlock(q->mutex);
    return size == N;
}
int blockq_push(BlockQ* q, int val) {
    pthread_mutex_lock(q->mutex);
    while (q->size == N) {
        pthread_cond_wait(q->not_full, q->mutex);
    }
    q->elements[q->rear] = val;
    q->rear = (q->rear + 1) % N;
    q->size++;
    pthread_cond_signal(q->not_empty);
    pthread_mutex_unlock(q->mutex);
    return 0;
}
int blockq_pop(BlockQ* q) {
    pthread_mutex_lock(q->mutex);
    while (q->size == 0) {
        pthread_cond_wait(q->not_empty, q->mutex);
    }
    int val = q->elements[q->front];
    q->front = (q->front + 1) % N;
    q->size--;
    pthread_cond_signal(q->not_full);
    pthread_mutex_unlock(q->mutex);
    return val;
}
int blockq_peek(BlockQ* q) {
    pthread_mutex_lock(q->mutex);
    if (q->size == 0) {
        pthread_mutex_unlock(q->mutex);
        return -1;
    }
    int val = q->elements[q->front];
    pthread_mutex_unlock(q->mutex);
    return val;
}
