#include "mutex_ring_buffer.h"
#include <string.h>

void mutex_ring_buffer_init(MutexRingBuffer *rb) {
    memset(rb->buffer, 0, sizeof(rb->buffer));
    rb->head = 0;
    rb->tail = 0;
    pthread_mutex_init(&rb->lock, NULL);
}

int mutex_ring_buffer_push(MutexRingBuffer *rb, const Tick *tick) {
    pthread_mutex_lock(&rb->lock);

    if (rb->head - rb->tail >= MUTEX_RING_CAPACITY) {
        pthread_mutex_unlock(&rb->lock);
        return 0; // full
    }

    rb->buffer[rb->head & (MUTEX_RING_CAPACITY - 1)] = *tick;
    rb->head++;

    pthread_mutex_unlock(&rb->lock);
    return 1;
}

int mutex_ring_buffer_pop(MutexRingBuffer *rb, Tick *out_tick) {
    pthread_mutex_lock(&rb->lock);

    if (rb->tail == rb->head) {
        pthread_mutex_unlock(&rb->lock);
        return 0; // empty
    }

    *out_tick = rb->buffer[rb->tail & (MUTEX_RING_CAPACITY - 1)];
    rb->tail++;

    pthread_mutex_unlock(&rb->lock);
    return 1;
}

void mutex_ring_buffer_destroy(MutexRingBuffer *rb) {
    pthread_mutex_destroy(&rb->lock);
}