// #include "worker_args.h"
// #include <stddef.h>

// void *consumer_thread(void *arg) {
//     ConsumerArgs *args = (ConsumerArgs *)arg;
//     Tick tick;

//     for (;;) {
//         if (ring_buffer_pop(args->rb, &tick)) {
//             order_book_apply(args->ob, &tick);
//             args->stats->ticks_processed++;
//         } else if (*(args->stop_flag)) {
//             if (!ring_buffer_pop(args->rb, &tick)) {
//                 // args->stats->drops++;
//                 break;
//             }
//             order_book_apply(args->ob, &tick);
//             args->stats->ticks_processed++;
//         }
//     }

//     return NULL;
// }
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
            // Producer is done. Keep draining until the buffer is
            // TRULY empty — don't assume only one tick could be left.
            int drained_more = 0;
            while (ring_buffer_pop(args->rb, &tick)) {
                order_book_apply(args->ob, &tick);
                args->stats->ticks_processed++;
                drained_more = 1;
            }
            (void)drained_more;
            break;
        }
        // else: buffer momentarily empty, producer still running -> spin and retry
    }

    return NULL;
}