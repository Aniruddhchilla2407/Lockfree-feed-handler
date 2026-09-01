#include "mutex_ring_buffer.h"
#include "tick.h"
#include "stats.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_TRIALS 15

typedef struct {
    MutexRingBuffer *rb;
    int               count;
    volatile int     *stop_flag;
    volatile int     *start_flag;
} ProdCtx;

typedef struct {
    MutexRingBuffer *rb;
    int               count;
    volatile int     *stop_flag;
    volatile int     *start_flag;
    uint64_t          received;
    uint64_t         *latencies_ns;
} ConsCtx;

static void *producer(void *arg) {
    ProdCtx *ctx = (ProdCtx *)arg;
    Tick t;
    memset(&t, 0, sizeof(t));
    strncpy(t.symbol, "BENCH", SYMBOL_LEN - 1);

    while (!*(ctx->start_flag)) { /* spin until released */ }

    for (int i = 0; i < ctx->count; i++) {
        t.price = (double)i;
        t.timestamp_ns = stats_now_ns();
        while (!mutex_ring_buffer_push(ctx->rb, &t)) { /* retry */ }
    }
    *(ctx->stop_flag) = 1;
    return NULL;
}

static void *consumer(void *arg) {
    ConsCtx *ctx = (ConsCtx *)arg;
    Tick t;
    uint64_t received = 0;

    while (!*(ctx->start_flag)) { /* spin until released */ }

    for (;;) {
        if (mutex_ring_buffer_pop(ctx->rb, &t)) {
            uint64_t now = stats_now_ns();
            ctx->latencies_ns[received] = now - t.timestamp_ns;
            received++;
        } else if (*(ctx->stop_flag)) {
            while (mutex_ring_buffer_pop(ctx->rb, &t)) {
                uint64_t now = stats_now_ns();
                ctx->latencies_ns[received] = now - t.timestamp_ns;
                received++;
            }
            break;
        }
    }
    ctx->received = received;
    return NULL;
}

static double run_once(int tick_count, uint64_t *latencies_ns) {
    MutexRingBuffer rb;
    mutex_ring_buffer_init(&rb);
    volatile int stop_flag = 0;
    volatile int start_flag = 0;

    ProdCtx pctx = { &rb, tick_count, &stop_flag, &start_flag };
    ConsCtx cctx = { &rb, tick_count, &stop_flag, &start_flag, 0, latencies_ns };

    pthread_t p_tid, c_tid;

    pthread_create(&c_tid, NULL, consumer, &cctx);
    pthread_create(&p_tid, NULL, producer, &pctx);

    uint64_t start = stats_now_ns();
    start_flag = 1;

    pthread_join(p_tid, NULL);
    pthread_join(c_tid, NULL);

    uint64_t end = stats_now_ns();
    double elapsed_sec = (double)(end - start) / 1e9;

    mutex_ring_buffer_destroy(&rb);
    return (double)tick_count / elapsed_sec;
}

static int cmp_double(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

static int cmp_uint64(const void *a, const void *b) {
    uint64_t ua = *(const uint64_t *)a;
    uint64_t ub = *(const uint64_t *)b;
    return (ua > ub) - (ua < ub);
}

#define LATENCY_TRIALS 5   // separate, smaller count just for latency combination (memory)

int main(void) {
    int sizes[] = { 100000, 500000, 1000000, 5000000, 10000000 };
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    FILE *f = fopen("bench/results/lockfree.csv", "w");
    if (!f) {
        fprintf(stderr, "Could not open bench/results/lockfree.csv for writing\n");
        return 1;
    }
    fprintf(f, "tick_count,ticks_per_sec,p50_ns,p99_ns\n");

    printf("%-12s %-20s %-12s %-12s\n", "tick_count", "ticks/sec (median)", "p50 (ns)", "p99 (ns)");

    for (int i = 0; i < num_sizes; i++) {
        double trials[NUM_TRIALS];

        // Buffer holds latencies from LATENCY_TRIALS runs combined (not all NUM_TRIALS,
        // to keep memory reasonable at large tick counts).
        uint64_t *all_latencies = malloc(sizeof(uint64_t) * (size_t)sizes[i] * LATENCY_TRIALS);
        if (!all_latencies) {
            fprintf(stderr, "Allocation failure at size %d\n", sizes[i]);
            return 1;
        }

        for (int t = 0; t < NUM_TRIALS; t++) {
            if (t < LATENCY_TRIALS) {
                uint64_t *trial_latencies = all_latencies + ((size_t)t * sizes[i]);
                trials[t] = run_once(sizes[i], trial_latencies);
            } else {
                // still measure throughput, but discard latencies (reuse a throwaway trial-0 slice)
                trials[t] = run_once(sizes[i], all_latencies);
            }
        }

        qsort(trials, NUM_TRIALS, sizeof(double), cmp_double);
        double median_tps = trials[NUM_TRIALS / 2];

        size_t total_latencies = (size_t)sizes[i] * LATENCY_TRIALS;
        qsort(all_latencies, total_latencies, sizeof(uint64_t), cmp_uint64);
        uint64_t p50 = all_latencies[total_latencies / 2];
        uint64_t p99 = all_latencies[(size_t)(total_latencies * 0.99)];

        printf("%-12d %-20.0f %-12llu %-12llu\n", sizes[i], median_tps,
               (unsigned long long)p50, (unsigned long long)p99);
        fprintf(f, "%d,%.0f,%llu,%llu\n", sizes[i], median_tps,
                (unsigned long long)p50, (unsigned long long)p99);

        free(all_latencies);
    }

    fclose(f);
    printf("Results written to bench/results/mutex.csv\n");
    return 0;
}