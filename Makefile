CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic
CPPFLAGS ?= -Iinclude -Isrc

BUILD_DIR := build
TEST_API_BIN := $(BUILD_DIR)/test_api_compile
TEST_TIMING_BIN := $(BUILD_DIR)/test_input_timing
TEST_BINS := $(TEST_API_BIN) $(TEST_TIMING_BIN)

.PHONY: all test clean dynamic-memory-scan

all: $(TEST_BINS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_API_BIN): src/ppg_ibi.c tests/test_api_compile.c include/ppg_ibi.h include/ppg_ibi_config.h src/ppg_ibi_internal.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/ppg_ibi.c tests/test_api_compile.c -o $(TEST_API_BIN)

$(TEST_TIMING_BIN): src/ppg_ibi.c tests/test_input_timing.c include/ppg_ibi.h include/ppg_ibi_config.h src/ppg_ibi_internal.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/ppg_ibi.c tests/test_input_timing.c -o $(TEST_TIMING_BIN)

dynamic-memory-scan:
	@if grep -R -n -E '\b(malloc|calloc|realloc)\s*\(' include src tests; then \
		echo "Dynamic memory API usage found"; \
		exit 1; \
	fi

test: $(TEST_BINS) dynamic-memory-scan
	./$(TEST_API_BIN)
	./$(TEST_TIMING_BIN)

clean:
	rm -rf $(BUILD_DIR)
