#ifndef _RT_CLIENT_QUEUE_H_
#define _RT_CLIENT_QUEUE_H_

#include "rt_socket.h"

#include <stdint.h>
#include <pthread.h>

#define QUEUE_CAPACITY 256

typedef struct {
    rt_socket clients[QUEUE_CAPACITY];

    // Queue data
    int32_t head;
    int32_t tail;
    int32_t count;

    // Thread data
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} rt_client_queue;

void rt_init_client_queue(rt_client_queue *q);
void rt_queue_push(rt_client_queue *q, rt_socket client_fd);
rt_socket rt_queue_pop(rt_client_queue *q);

#endif // _RT_CLIENT_QUEUE_H_