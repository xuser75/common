# Makefile for Common compiler test suite and examples

CC = gcc
CFLAGS = -std=c99 -Wall -O2
NASM = nasm
NASMFLAGS = -f elf32
LD = gcc
LDFLAGS = -m32

# Compiler
COMPILER = common
COMPILER_SRC = common.c

# Test runner
TEST_RUNNER = test_runner
TEST_RUNNER_SRC = test_runner.c

# Example programs
EXAMPLES_DIR = examples
EXAMPLES = hello fibonacci arrays pointers bubblesort bitwise types switch primes strings calculator

# Default target
.PHONY: all
all: $(COMPILER) $(TEST_RUNNER)

# Build the Common compiler
$(COMPILER): $(COMPILER_SRC)
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Built Common compiler"

# Build the test runner
$(TEST_RUNNER): $(TEST_RUNNER_SRC)
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Built test runner"

# Run tests
.PHONY: test
test: $(COMPILER) $(TEST_RUNNER)
	@echo "Running test suite..."
	./$(TEST_RUNNER)

# Build all examples
.PHONY: examples
examples: $(COMPILER) $(EXAMPLES)

# Pattern rule for building examples
# Usage: make hello, make fibonacci, etc.
$(EXAMPLES): %: $(EXAMPLES_DIR)/%.cm $(COMPILER)
	@echo "Building $@..."
	./$(COMPILER) $(EXAMPLES_DIR)/$@.cm $@.asm
	$(NASM) $(NASMFLAGS) $@.asm -o $@.o
	$(LD) $(LDFLAGS) $@.o -o $@ -no-pie
	@echo "Built $@ successfully"

# Run all examples
.PHONY: run-examples
run-examples: examples
	@echo "=== Running Examples ==="
	@for prog in $(EXAMPLES); do \
		echo ""; \
		echo ">>> Running $$prog..."; \
		./$$prog || true; \
	done

# Clean all generated files
.PHONY: clean
clean:
	rm -f $(COMPILER) $(TEST_RUNNER)
	rm -f *.asm *.o $(EXAMPLES)
	rm -f /tmp/test.cm /tmp/test.asm /tmp/test.o /tmp/test /tmp/test.out /tmp/test.err
	@echo "Cleaned all generated files"

# Install to /usr/local/
.PHONY: install
install:
	install common /usr/local/bin/
	@echo Installed common to /usr/local/bin/

# Help
.PHONY: help
help:
	@echo "Common Compiler Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build compiler and test runner (default)"
	@echo "  test          - Run the test suite"
	@echo "  examples      - Build all example programs"
	@echo "  run-examples  - Build and run all examples"
	@echo "  clean         - Remove all generated files"
	@echo "  install       - Install to /usr/local/bin"
	@echo ""
	@echo "Build individual examples:"
	@echo "  make hello"
	@echo "  make fibonacci"
	@echo "  make arrays"
	@echo "  make pointers"
	@echo "  make bubblesort"
	@echo "  make bitwise"
	@echo "  make types"
	@echo "  make switch"
	@echo "  make primes"
	@echo "  make strings"
	@echo "  make calculator"
	@echo ""
	@echo "Run individual examples:"
	@echo "  make hello && ./hello"
	@echo "  make fibonacci && ./fibonacci"
	@echo "  etc."
