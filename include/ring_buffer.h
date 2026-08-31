#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "tick.h"
#include <stdatomic.h>
#include <stdint.h>

#define RING_CAPACITY 1024   // must be a power of 2

// Single-Producer Single-Consumer lock-free ring buffer.
// One thread calls push(), a different single thread calls pop().
// Not safe for multiple producers or multiple consumers at once.
typedef struct {
    Tick    buffer[RING_CAPACITY];

    // Padded to separate cache lines: head and tail are written by
    // different threads, and without padding they'd sit on the same
    // cache line, causing false sharing (each thread's write would
    // invalidate the other core's cache for no real reason).
    _Alignas(64) atomic_uint_fast64_t head;   // next write index (producer-owned)
    _Alignas(64) atomic_uint_fast64_t tail;   // next read index  (consumer-owned)
} RingBuffer;

void ring_buffer_init(RingBuffer *rb);

// Returns 1 on success, 0 if buffer is full (producer must retry/drop).
int ring_buffer_push(RingBuffer *rb, const Tick *tick);

// Returns 1 on success, 0 if buffer is empty (consumer must retry/wait).
int ring_buffer_pop(RingBuffer *rb, Tick *out_tick);

#endif // RING_BUFFER_H