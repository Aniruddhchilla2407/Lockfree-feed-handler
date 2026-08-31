#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "tick.h"
#include <stdint.h>

#define MAX_SYMBOLS 64

typedef struct {
    char     symbol[SYMBOL_LEN];
    double   best_bid;
    double   best_ask;
    uint64_t update_count;
} SymbolBook;

typedef struct {
    SymbolBook books[MAX_SYMBOLS];
    int        num_symbols;
} OrderBook;

void order_book_init(OrderBook *ob);

// Applies a tick to the book: updates best bid/ask for that symbol.
// Adds the symbol to the book if it hasn't been seen before.
void order_book_apply(OrderBook *ob, const Tick *tick);

// Debug/inspection helper
void order_book_print(const OrderBook *ob);

#endif // ORDER_BOOK_H