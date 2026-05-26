CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude -std=gnu99 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE -Wno-deprecated-declarations

LIB_SRC = src/uthread.c src/scheduler.c src/queue.c
LIB_OBJ = $(patsubst src/%.c, build/%.o, $(LIB_SRC))

TESTS     = test_queue test_basic test_fifo test_rr test_priority test_join test_stress
TEST_BINS = $(addprefix build/, $(TESTS))

DEMOS     = demo_uthread
DEMO_BINS = $(addprefix build/, $(DEMOS))

.PHONY: all test clean demo \
        test_queue test_basic test_fifo test_rr test_priority test_join test_stress

# ── Default target ────────────────────────────────────────────────────────
all: $(TEST_BINS) $(DEMO_BINS)

# ── Build directory ───────────────────────────────────────────────────────
build:
	@mkdir -p build

# ── Library object files ──────────────────────────────────────────────────
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# ── Test binaries (each links the full library) ───────────────────────────
build/test_%: tests/test_%.c $(LIB_OBJ) | build
	$(CC) $(CFLAGS) $^ -o $@

# ── Demo binaries ─────────────────────────────────────────────────────────
build/demo_%: tests/demo_%.c $(LIB_OBJ) | build
	$(CC) $(CFLAGS) $^ -o $@

# ── Run all tests ─────────────────────────────────────────────────────────
test: all
	@failed=0; \
	for t in $(TEST_BINS); do \
		echo ""; \
		echo "=== $$t ==="; \
		./$$t || failed=1; \
	done; \
	exit $$failed

# ── Run individual tests ──────────────────────────────────────────────────
test_queue:    build/test_queue    ; @./build/test_queue
test_basic:    build/test_basic    ; @./build/test_basic
test_fifo:     build/test_fifo     ; @./build/test_fifo
test_rr:       build/test_rr       ; @./build/test_rr
test_priority: build/test_priority ; @./build/test_priority
test_join:     build/test_join     ; @./build/test_join
test_stress:   build/test_stress   ; @./build/test_stress

# ── Run demo ─────────────────────────────────────────────────────────────
demo: build/demo_uthread
	@./build/demo_uthread

# ── Clean ─────────────────────────────────────────────────────────────────
clean:
	@rm -rf build/
	@echo "Cleaned."
