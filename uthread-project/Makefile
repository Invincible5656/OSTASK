CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude -D_XOPEN_SOURCE

LIB_SRC = src/uthread.c src/scheduler.c src/queue.c
LIB_OBJ = $(patsubst src/%.c, build/%.o, $(LIB_SRC))

TESTS     = test_basic test_fifo test_rr test_priority test_join test_stress
TEST_BINS = $(addprefix build/, $(TESTS))

.PHONY: all test clean \
        test_basic test_fifo test_rr test_priority test_join test_stress

# ── Default target ────────────────────────────────────────────────────────
all: $(TEST_BINS)

# ── Build directory ───────────────────────────────────────────────────────
build:
	@mkdir -p build

# ── Library object files ──────────────────────────────────────────────────
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# ── Test binaries (each links the full library) ───────────────────────────
build/test_%: tests/test_%.c $(LIB_OBJ) | build
	$(CC) $(CFLAGS) $^ -o $@

# ── Run all tests ─────────────────────────────────────────────────────────
test: all
	@for t in $(TEST_BINS); do \
		echo ""; \
		echo "=== $$t ==="; \
		./$$t || true; \
	done

# ── Run individual tests ──────────────────────────────────────────────────
test_basic:    build/test_basic    ; @./build/test_basic
test_fifo:     build/test_fifo     ; @./build/test_fifo
test_rr:       build/test_rr       ; @./build/test_rr
test_priority: build/test_priority ; @./build/test_priority
test_join:     build/test_join     ; @./build/test_join
test_stress:   build/test_stress   ; @./build/test_stress

# ── Clean ─────────────────────────────────────────────────────────────────
clean:
	@rm -rf build/
	@echo "Cleaned."
