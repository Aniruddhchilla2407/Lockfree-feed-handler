CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11 -Iinclude
LDFLAGS := -lpthread

SRC_DIR := src
BUILD_DIR := build
TEST_DIR := tests
BIN := feed_handler

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run test bench

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(BIN) 1000000

test: $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_ring_buffer.c $(SRC_DIR)/ring_buffer.c -o $(BUILD_DIR)/test_ring_buffer $(LDFLAGS)
	$(CC) $(CFLAGS) $(TEST_DIR)/test_order_book.c $(SRC_DIR)/order_book.c -o $(BUILD_DIR)/test_order_book
	@echo "--- Running test_ring_buffer ---"
	./$(BUILD_DIR)/test_ring_buffer
	@echo "--- Running test_order_book ---"
	./$(BUILD_DIR)/test_order_book

bench: $(BUILD_DIR)
	$(CC) $(CFLAGS) bench/bench_lockfree.c $(SRC_DIR)/ring_buffer.c $(SRC_DIR)/stats.c -o $(BUILD_DIR)/bench_lockfree $(LDFLAGS)
	$(CC) $(CFLAGS) -Ibaseline bench/bench_mutex.c baseline/mutex_ring_buffer.c $(SRC_DIR)/stats.c -o $(BUILD_DIR)/bench_mutex $(LDFLAGS)
	@echo "--- Running lock-free benchmark ---"
	./$(BUILD_DIR)/bench_lockfree
	@echo "--- Running mutex benchmark ---"
	./$(BUILD_DIR)/bench_mutex

clean:
	rm -rf $(BUILD_DIR) $(BIN)