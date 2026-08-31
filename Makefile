CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11 -Iinclude
LDFLAGS := -lpthread

SRC_DIR := src
BUILD_DIR := build
BIN := feed_handler

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(BIN) 1000000

clean:
	rm -rf $(BUILD_DIR) $(BIN)