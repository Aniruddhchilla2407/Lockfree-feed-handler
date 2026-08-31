#include "worker_args.h"
#include <stddef.h>

void *consumer_thread(void *arg) {
    ConsumerArgs *args = (ConsumerArgs *)arg;
    Tick tick;

    for (;;) {
        if (ring_buffer_pop(args->rb, &tick)) {
            order_book_apply(args->ob, &tick);
            args->stats->ticks_processed++;
        } else if (*(args->stop_flag)) {
            if (!ring_buffer_pop(args->rb, &tick)) {
                // args->stats->drops++;
                break;
            }
            order_book_apply(args->ob, &tick);
            args->stats->ticks_processed++;
        }
    }

    return NULL;
}