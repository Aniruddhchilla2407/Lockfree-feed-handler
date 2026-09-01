#include "order_book.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(int condition, const char *msg) {
    if (!condition) {
        printf("FAIL: %s\n", msg);
        failures++;
    }
}

static Tick make_tick(const char *symbol, double price, Side side) {
    Tick t;
    memset(&t, 0, sizeof(t));
    strncpy(t.symbol, symbol, SYMBOL_LEN - 1);
    t.price = price;
    t.quantity = 1;
    t.side = side;
    return t;
}

int main(void) {
    OrderBook ob;
    order_book_init(&ob);

    // 1. New symbol added on first tick
    Tick t1 = make_tick("AAPL", 150.0, SIDE_BID);
    order_book_apply(&ob, &t1);
    check(ob.num_symbols == 1, "AAPL should be added as first symbol");
    check(ob.books[0].best_bid == 150.0, "AAPL best_bid should be 150.0 after first bid tick");
    check(ob.books[0].best_ask == 0.0, "AAPL best_ask should be untouched by a bid tick");

    // 2. Ask tick sets best_ask without touching best_bid
    Tick t2 = make_tick("AAPL", 151.0, SIDE_ASK);
    order_book_apply(&ob, &t2);
    check(ob.books[0].best_ask == 151.0, "AAPL best_ask should be 151.0 after ask tick");
    check(ob.books[0].best_bid == 150.0, "AAPL best_bid should remain 150.0 after unrelated ask tick");

    // 3. Later tick overwrites previous value (proves min/max bug is gone)
    Tick t3 = make_tick("AAPL", 100.0, SIDE_BID); // lower than previous bid
    order_book_apply(&ob, &t3);
    check(ob.books[0].best_bid == 100.0, "AAPL best_bid should overwrite to 100.0, not stay at max (150.0)");

    // 4. Second distinct symbol tracked independently
    Tick t4 = make_tick("MSFT", 300.0, SIDE_BID);
    order_book_apply(&ob, &t4);
    check(ob.num_symbols == 2, "MSFT should be added as second symbol");
    check(ob.books[1].best_bid == 300.0, "MSFT best_bid should be 300.0");
    check(ob.books[0].best_bid == 100.0, "AAPL best_bid should be unaffected by MSFT tick");

    // 5. update_count increments correctly
    check(ob.books[0].update_count == 3, "AAPL should have 3 updates (2 bids + 1 ask)");
    check(ob.books[1].update_count == 1, "MSFT should have 1 update");

    if (failures == 0) {
        printf("PASS: all order_book tests passed\n");
        return 0;
    } else {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
}