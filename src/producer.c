#include "worker_args.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *SYMBOLS[] = { "AAPL", "MSFT", "GOOG", "TSLA" };
#define NUM_SYMBOLS (sizeof(SYMBOLS) / sizeof(SYMBOLS[0]))

void *producer_thread(void *arg) {
    ProducerArgs *args = (ProducerArgs *)arg;

    for (int i = 0; i < args->num_ticks; i++) {
        Tick tick;
        memset(&tick, 0, sizeof(tick));

        const char *sym = SYMBOLS[rand() % NUM_SYMBOLS];
        strncpy(tick.symbol, sym, SYMBOL_LEN - 1);

        tick.price        = 100.0 + (rand() % 1000) / 10.0;
        tick.quantity     = 1 + (rand() % 500);
        tick.side         = (rand() % 2) ? SIDE_BID : SIDE_ASK;
        tick.timestamp_ns = stats_now_ns();

        if (!ring_buffer_push(args->rb, &tick)) {
            args->stats->drops++;
        }
    }

    *(args->stop_flag) = 1;
    return NULL;
}