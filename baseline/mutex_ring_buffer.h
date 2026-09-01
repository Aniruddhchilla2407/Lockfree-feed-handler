#ifndef MUTEX_RING_BUFFER_H
#define MUTEX_RING_BUFFER_H

#include "tick.h"
#include <pthread.h>
#include <stdint.h>

#define MUTEX_RING_CAPACITY 1024

// Same semantics as the lock-free RingBuffer, but protected by a mutex.
// Used purely as a performance baseline for comparison.
typedef struct {
    Tick            buffer[MUTEX_RING_CAPACITY];
    uint64_t        head;
    uint64_t        tail;
    pthread_mutex_t lock;
} MutexRingBuffer;

void mutex_ring_buffer_init(MutexRingBuffer *rb);
int  mutex_ring_buffer_push(MutexRingBuffer *rb, const Tick *tick);
int  mutex_ring_buffer_pop(MutexRingBuffer *rb, Tick *out_tick);
void mutex_ring_buffer_destroy(MutexRingBuffer *rb);

#endif // MUTEX_RING_BUFFER_H