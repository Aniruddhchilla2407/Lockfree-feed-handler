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
make run                # builds and runs with 1,000,000 simulated ticks
./feed_handler 5000000  # or run manually with a custom tick count
make test                # runs correctness tests (ring buffer + order book)
make bench                # runs lock-free vs. mutex throughput benchmark
```

Requires a C11 compiler with `<stdatomic.h>` support and pthreads (tested on
Debian/WSL2 with gcc).

## Correctness tests

- **`tests/test_ring_buffer.c`** — spawns a real producer/consumer thread pair
  and pushes 500,000 sequential values through the lock-free ring buffer,
  verifying every value is received exactly once, in order, with no loss or
  corruption.
- **`tests/test_order_book.c`** — verifies order book logic in isolation
  (new symbol insertion, independent bid/ask tracking per side, correct
  overwrite-on-update semantics, per-symbol update counts).

Both are wired into `make test` and run automatically in CI on every push.

## Benchmark: lock-free vs. mutex-protected ring buffer

To quantify the actual benefit of the lock-free design, `baseline/` contains
a mutex-protected ring buffer with an identical interface. `bench/` runs both
implementations through the same producer/consumer workload across a range of
tick counts, with **15 trials per size, reporting the median** (a single-trial
measurement showed significant run-to-run jitter under WSL2's scheduler;
median-of-15 gives a stable, trustworthy number).

| Ticks       | Lock-free (ticks/sec) | Mutex (ticks/sec) | Speedup |
|-------------|------------------------|--------------------|---------|
| 100,000     | 19,467,016             | 6,226,267          | ~3.1x   |
| 500,000     | 21,468,434             | 6,399,204          | ~3.4x   |
| 1,000,000   | 21,153,368             | 6,589,977          | ~3.2x   |
| 5,000,000   | 37,665,578             | 7,076,579          | ~5.3x   |
| 10,000,000  | 39,100,446             | 6,677,993          | ~5.8x   |

The lock-free implementation consistently outperforms the mutex baseline by
3-6x, with the gap widening at higher throughput — consistent with mutex
contention overhead scaling with operation count, while the lock-free path
has no contention to begin with.

Raw results: `bench/results/lockfree.csv`, `bench/results/mutex.csv`.

### End-to-end latency (per-tick push-to-pop time)

| Ticks       | Lock-free p50 (ns) | Lock-free p99 (ns) | Mutex p50 (ns) | Mutex p99 (ns) |
|-------------|---------------------|---------------------|-----------------|-----------------|
| 100,000     | 17,387              | 79,660              | 56,528          | 3,525,047       |
| 500,000     | 372                 | 78,089              | 37,163          | 463,698         |
| 1,000,000   | 23,709              | 66,316              | 77,595          | 2,062,048       |
| 5,000,000   | 14,207              | 68,924              | 96,062          | 622,597         |
| 10,000,000  | 7,340               | 81,227              | 97,331          | 381,707         |

The lock-free implementation's p99 latency is consistently far lower and
more stable than the mutex baseline's — the mutex baseline shows occasional
severe tail-latency spikes (e.g. 3.5ms at 100K ticks), consistent with a
thread occasionally blocking on lock contention and being descheduled by the
OS for an extended period. The lock-free version has no such blocking path,
so its p99 stays in a tight band (~66-81μs) regardless of load. Lock-free p50
is also generally lower but noisier run-to-run, likely because its busy-spin
consumer can catch a push almost immediately when already spinning, making
best-case timing sensitive to scheduling luck. p99 is the more meaningful
comparison for a latency-sensitive system, since it reflects worst-case
behavior rather than best-case timing.

## Debugging notes

A few real bugs were found and fixed during development, documented here
because the process is arguably more informative than the final code:

- **Order book logic bug**: initial `order_book_apply` treated best bid/ask
  as a running max/min across all ticks ever seen, causing every symbol to
  converge to the same extreme values. Fixed to treat each tick as the
  current quote for its side (a direct overwrite), matching real order book
  semantics.
- **Shutdown drain race**: the consumer's exit logic originally attempted
  only one extra `pop` after the producer signaled completion, which could
  leave ticks stranded in the ring buffer if more than one was left. Fixed
  by looping until the buffer is verifiably empty before exiting.
- **Benchmark measurement noise**: initial single-trial benchmarks showed the
  lock-free implementation swinging 2-3x between runs of the same tick count,
  even with the same binary run back-to-back. Traced to (a) thread-creation
  overhead being included in the timed region at small sizes, and (b) OS
  scheduling jitter under WSL2. Fixed by starting the timer only after both
  threads are created and synchronized on a start flag, and by reporting the
  median of 15 trials per size instead of a single sample.
