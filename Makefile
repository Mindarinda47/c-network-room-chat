CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS ?= -Isrc/common
BUILD_DIR := build

COMMON_OBJ := $(BUILD_DIR)/protocol.o

.PHONY: all clean test

all: $(BUILD_DIR)/chat-server $(BUILD_DIR)/chat-client

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/protocol.o: src/common/protocol.c src/common/protocol.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/chat-server: src/server/server.c $(COMMON_OBJ) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/chat-client: src/client/client.c $(COMMON_OBJ) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/protocol-test: tests/protocol_test.c $(COMMON_OBJ) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

test: all $(BUILD_DIR)/protocol-test
	$(BUILD_DIR)/protocol-test
	python3 tests/integration_test.py

clean:
	rm -rf $(BUILD_DIR)
