#include "ring_buffer.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_TICK_COUNT 500000

typedef struct {
    RingBuffer *rb;
    int         count;
} ProducerCtx;

typedef struct {
    RingBuffer *rb;
    int         count;
    double     *received;   // array to record every price we pop, in order
    int         received_count;
} ConsumerCtx;

static void *producer_fn(void *arg) {
    ProducerCtx *ctx = (ProducerCtx *)arg;

    for (int i = 0; i < ctx->count; i++) {
        Tick t;
        memset(&t, 0, sizeof(t));
        strncpy(t.symbol, "TEST", SYMBOL_LEN - 1);
        t.price = (double)i;   // known sequential value
        t.quantity = 1;
        t.side = SIDE_BID;

        while (!ring_buffer_push(ctx->rb, &t)) {
            // retry until it fits
        }
    }
    return NULL;
}

static void *consumer_fn(void *arg) {
    ConsumerCtx *ctx = (ConsumerCtx *)arg;
    Tick t;
    int received = 0;

    while (received < ctx->count) {
        if (ring_buffer_pop(ctx->rb, &t)) {
            ctx->received[received] = t.price;
            received++;
        }
    }
    ctx->received_count = received;
    return NULL;
}

int main(void) {
    RingBuffer rb;
    ring_buffer_init(&rb);

    double *received = malloc(sizeof(double) * TEST_TICK_COUNT);
    if (!received) {
        fprintf(stderr, "FAIL: allocation error\n");
        return 1;
    }

    ProducerCtx pctx = { &rb, TEST_TICK_COUNT };
    ConsumerCtx cctx = { &rb, TEST_TICK_COUNT, received, 0 };

    pthread_t p_tid, c_tid;
    pthread_create(&c_tid, NULL, consumer_fn, &cctx);
    pthread_create(&p_tid, NULL, producer_fn, &pctx);

    pthread_join(p_tid, NULL);
    pthread_join(c_tid, NULL);

    // Verify: correct count, correct order, no duplicates/corruption
    int pass = 1;
    if (cctx.received_count != TEST_TICK_COUNT) {
        printf("FAIL: expected %d ticks, got %d\n", TEST_TICK_COUNT, cctx.received_count);
        pass = 0;
    }

    for (int i = 0; i < cctx.received_count; i++) {
        if (received[i] != (double)i) {
            printf("FAIL: at index %d, expected price %d, got %.1f\n", i, i, received[i]);
            pass = 0;
            break; // one mismatch is enough to prove a problem
        }
    }

    free(received);

    if (pass) {
        printf("PASS: all %d ticks received in correct order, no loss or corruption\n", TEST_TICK_COUNT);
        return 0;
    } else {
        return 1;
    }
}