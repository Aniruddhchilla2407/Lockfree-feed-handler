#include "ring_buffer.h"
#include "order_book.h"
#include "stats.h"
#include "worker_args.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

extern void *producer_thread(void *arg);
extern void *consumer_thread(void *arg);

int main(int argc, char **argv) {
    int num_ticks = (argc > 1) ? atoi(argv[1]) : 1000000;

    RingBuffer rb;
    OrderBook ob;
    Stats stats;
    volatile int stop_flag = 0;

    ring_buffer_init(&rb);
    order_book_init(&ob);
    stats_init(&stats);

    ProducerArgs pargs = { &rb, &stats, num_ticks, &stop_flag };
    ConsumerArgs cargs = { &rb, &ob, &stats, &stop_flag };

    pthread_t producer_tid, consumer_tid;

    stats_mark_start(&stats);

    pthread_create(&consumer_tid, NULL, consumer_thread, &cargs);
    pthread_create(&producer_tid, NULL, producer_thread, &pargs);

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    stats_mark_end(&stats);

    printf("\n");
    order_book_print(&ob);
    printf("\n");
    stats_report(&stats);

    return 0;
}