#include "rt_client_queue.h"

void rt_init_client_queue(
    rt_client_queue *q
) {
    q->head = q->tail = q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void rt_queue_push(
    rt_client_queue *q,
    rt_socket client_fd
) {
    pthread_mutex_lock(&q->mutex);

    while (q->count == QUEUE_CAPACITY)
        pthread_cond_wait(&q->not_full, &q->mutex);

    q->clients[q->tail] = client_fd;
    q->tail = (q->tail + 1) % QUEUE_CAPACITY;
    q->count++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

rt_socket rt_queue_pop(
    rt_client_queue *q
) {
    pthread_mutex_lock(&q->mutex);

    while (q->count == 0)
        pthread_cond_wait(&q->not_empty, &q->mutex);

    rt_socket fd = q->clients[q->head];
    q->head = (q->head + 1) % QUEUE_CAPACITY;
    q->count--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return fd;
}