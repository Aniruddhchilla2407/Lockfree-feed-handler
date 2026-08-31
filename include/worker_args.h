#ifndef WORKER_ARGS_H
#define WORKER_ARGS_H

#include "ring_buffer.h"
#include "order_book.h"
#include "stats.h"

typedef struct {
    RingBuffer   *rb;
    Stats        *stats;
    int           num_ticks;
    volatile int *stop_flag;
} ProducerArgs;

typedef struct {
    RingBuffer   *rb;
    OrderBook    *ob;
    Stats        *stats;
    volatile int *stop_flag;
} ConsumerArgs;

#endif // WORKER_ARGS_H