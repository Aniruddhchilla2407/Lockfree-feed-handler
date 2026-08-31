# Lock-Free Market Data Feed Handler

A simulated low-latency market data pipeline in C, built to explore lock-free
concurrent programming patterns used in HFT infrastructure.

## What it does

- A **producer thread** generates simulated tick data (symbol, price, quantity, side)
  for a small set of symbols (AAPL, MSFT, GOOG, TSLA).
- Ticks are pushed into a **lock-free SPSC ring buffer** — no mutexes, using C11
  atomics with acquire/release memory ordering and cache-line-padded head/tail
  indices to avoid false sharing.
- A **consumer thread** pops ticks off the ring buffer and updates a per-symbol
  **order book** (current best bid / best ask).
- Basic throughput/latency stats are reported at the end of each run.

## Architecture
producer_thread --push--> RingBuffer (lock-free, SPSC) --pop--> consumer_thread
|
v
OrderBook

## Build & run

```bash
make run              # builds and runs with 1,000,000 simulated ticks
./feed_handler 5000000  # or run manually with a custom tick count
```

Requires a C11 compiler with `<stdatomic.h>` support and pthreads (tested on
Debian/WSL2 with gcc).

## Current results

At 1,000,000 ticks, throughput is in the range of ~6-7M ticks/sec on a single
producer/consumer thread pair.

## Known issues / in progress

- **Shutdown race condition**: when the producer finishes and sets the stop
  flag, the consumer's drain-on-exit logic currently only attempts one extra
  pop before exiting, which can leave ticks stranded in the ring buffer under
  certain timing conditions (observed data loss: ~1-2% of ticks in some runs).
  Fix in progress — consumer needs to loop until the buffer is fully drained,
  not just check once.
- Order book previously tracked running min/max instead of latest quote per
  side — since fixed; each tick now correctly overwrites the current best
  bid/ask for its side.

## Next steps

- Fix the shutdown drain race
- Add a baseline mutex-protected ring buffer for performance comparison
- Add correctness tests (no lost/corrupted ticks under load)
- Benchmark harness with CSV output for lock-free vs. mutex comparison