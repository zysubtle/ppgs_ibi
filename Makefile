CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic
CPPFLAGS ?= -Iinclude -Isrc

BUILD_DIR := build
TEST_BIN := $(BUILD_DIR)/test_api_compile

.PHONY: all test clean dynamic-memory-scan

all: $(TEST_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_BIN): src/ppg_ibi.c tests/test_api_compile.c include/ppg_ibi.h include/ppg_ibi_config.h src/ppg_ibi_internal.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/ppg_ibi.c tests/test_api_compile.c -o $(TEST_BIN)

dynamic-memory-scan:
	@if grep -R -n -E '\b(malloc|calloc|realloc)\s*\(' include src tests; then \
		echo "Dynamic memory API usage found"; \
		exit 1; \
	fi

test: $(TEST_BIN) dynamic-memory-scan
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
