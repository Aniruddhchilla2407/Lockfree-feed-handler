#include "ring_buffer.h"
#include <string.h>

void ring_buffer_init(RingBuffer *rb) {
    memset(rb->buffer, 0, sizeof(rb->buffer));
    atomic_store_explicit(&rb->head, 0, memory_order_relaxed);
    atomic_store_explicit(&rb->tail, 0, memory_order_relaxed);
}

int ring_buffer_push(RingBuffer *rb, const Tick *tick) {
    uint64_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    uint64_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);

    // Buffer full if writing here would catch up to consumer's tail
    if (head - tail >= RING_CAPACITY) {
        return 0;
    }

    rb->buffer[head & (RING_CAPACITY - 1)] = *tick;

    // release: ensures the write to buffer[] above is visible to the
    // consumer BEFORE it sees the updated head.
    atomic_store_explicit(&rb->head, head + 1, memory_order_release);
    return 1;
}

int ring_buffer_pop(RingBuffer *rb, Tick *out_tick) {
    uint64_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    uint64_t head = atomic_load_explicit(&rb->head, memory_order_acquire);

    // Buffer empty if consumer has caught up to producer
    if (tail == head) {
        return 0;
    }

    *out_tick = rb->buffer[tail & (RING_CAPACITY - 1)];

    // release: ensures the read above completes before the producer
    // is allowed to see the freed slot and overwrite it.
    atomic_store_explicit(&rb->tail, tail + 1, memory_order_release);
    return 1;
}