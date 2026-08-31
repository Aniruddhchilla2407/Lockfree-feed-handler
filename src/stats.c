#define _POSIX_C_SOURCE 199309L
#include "stats.h"
#include <stdio.h>
#include <time.h>

void stats_init(Stats *s) {
    s->ticks_processed = 0;
    s->drops = 0;
    s->start_time_ns = 0;
    s->end_time_ns = 0;
}

uint64_t stats_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void stats_mark_start(Stats *s) {
    s->start_time_ns = stats_now_ns();
}

void stats_mark_end(Stats *s) {
    s->end_time_ns = stats_now_ns();
}

void stats_report(const Stats *s) {
    uint64_t elapsed_ns = s->end_time_ns - s->start_time_ns;
    double elapsed_sec = (double)elapsed_ns / 1e9;
    double ticks_per_sec = elapsed_sec > 0 ? (double)s->ticks_processed / elapsed_sec : 0;

    printf("---- Stats ----\n");
    printf("Elapsed:        %.4f sec\n", elapsed_sec);
    printf("Ticks Generated:  %llu\n", (unsigned long long)(s->ticks_processed + s->drops));
    printf("Ticks processed: %llu\n", (unsigned long long)s->ticks_processed);
    printf("Ticks dropped:   %llu\n", (unsigned long long)s->drops);
    printf("Throughput:      %.0f ticks/sec\n", ticks_per_sec);
}