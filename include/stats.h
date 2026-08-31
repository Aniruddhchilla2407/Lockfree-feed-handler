#ifndef STATS_H
#define STATS_H

#include <stdint.h>

typedef struct {
    uint64_t ticks_processed;
    uint64_t drops;              // pushes that failed because buffer was full
    uint64_t start_time_ns;
    uint64_t end_time_ns;
} Stats;

void stats_init(Stats *s);

// Call once right before starting the producer/consumer threads.
void stats_mark_start(Stats *s);

// Call once after joining both threads.
void stats_mark_end(Stats *s);

// Prints ticks/sec, total processed, total dropped.
void stats_report(const Stats *s);

// Helper: current monotonic time in nanoseconds.
uint64_t stats_now_ns(void);

#endif // STATS_H