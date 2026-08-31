#ifndef TICK_H
#define TICK_H

#include <stdint.h>

#define SYMBOL_LEN 8

typedef enum {
    SIDE_BID = 0,
    SIDE_ASK = 1
} Side;

typedef struct {
    char     symbol[SYMBOL_LEN];   // e.g. "AAPL"
    double   price;
    uint32_t quantity;
    Side     side;
    uint64_t timestamp_ns;         // when the tick was generated
} Tick;

#endif // TICK_H