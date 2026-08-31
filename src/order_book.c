#include "order_book.h"
#include <stdio.h>
#include <string.h>

void order_book_init(OrderBook *ob) {
    memset(ob, 0, sizeof(*ob));
    ob->num_symbols = 0;
}

// Finds the SymbolBook for this tick's symbol, creating one if new.
static SymbolBook *find_or_create(OrderBook *ob, const char *symbol) {
    for (int i = 0; i < ob->num_symbols; i++) {
        if (strncmp(ob->books[i].symbol, symbol, SYMBOL_LEN) == 0) {
            return &ob->books[i];
        }
    }

    if (ob->num_symbols >= MAX_SYMBOLS) {
        return NULL; // book is full, drop silently
    }

    SymbolBook *sb = &ob->books[ob->num_symbols++];
    strncpy(sb->symbol, symbol, SYMBOL_LEN);
    sb->best_bid = 0.0;
    sb->best_ask = 0.0;
    sb->update_count = 0;
    return sb;
}

void order_book_apply(OrderBook *ob, const Tick *tick) {
    SymbolBook *sb = find_or_create(ob, tick->symbol);
    if (!sb) return;

    if (tick->side == SIDE_BID) {
        sb->best_bid = tick->price;
    } else {
        sb->best_ask = tick->price;
    }

    sb->update_count++;
}

void order_book_print(const OrderBook *ob) {
    for (int i = 0; i < ob->num_symbols; i++) {
        const SymbolBook *sb = &ob->books[i];
        printf("%-8s bid=%.2f ask=%.2f updates=%llu\n",
               sb->symbol, sb->best_bid, sb->best_ask,
               (unsigned long long)sb->update_count);
    }
}