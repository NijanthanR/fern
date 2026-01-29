# Fern Compiler - Makefile
# A statically-typed, functional language that compiles to single binaries

CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude -Ilib
DEBUGFLAGS = -g -O0 -DDEBUG
RELEASEFLAGS = -O2 -DNDEBUG
LDFLAGS =

# Directories
SRC_DIR = src
TEST_DIR = tests
LIB_DIR = lib
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin
RUNTIME_DIR = runtime

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)
RUNTIME_SRCS = $(wildcard $(RUNTIME_DIR)/*.c)

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BUILD_DIR)/test_%.o)
LIB_OBJS = $(LIB_SRCS:$(LIB_DIR)/%.c=$(BUILD_DIR)/lib_%.o)
RUNTIME_OBJS = $(RUNTIME_SRCS:$(RUNTIME_DIR)/%.c=$(BUILD_DIR)/runtime_%.o)

# Runtime library (linked into compiled Fern programs)
RUNTIME_LIB = $(BIN_DIR)/libfern_runtime.a

# Binaries
FERN_BIN = $(BIN_DIR)/fern
TEST_BIN = $(BIN_DIR)/test_runner

# Default target
.PHONY: all
all: debug

# Debug build
.PHONY: debug
debug: CFLAGS += $(DEBUGFLAGS)
debug: $(FERN_BIN) $(RUNTIME_LIB)

# Release build
.PHONY: release
release: CFLAGS += $(RELEASEFLAGS)
release: clean $(FERN_BIN) $(RUNTIME_LIB)

# Build fern compiler
$(FERN_BIN): $(OBJS) $(LIB_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Built fern compiler: $@"

# Build runtime library (static archive for linking into Fern programs)
$(RUNTIME_LIB): $(RUNTIME_OBJS) | $(BIN_DIR)
	ar rcs $@ $^
	@echo "✓ Built runtime library: $@"

# Build and run tests
.PHONY: test
test: CFLAGS += $(DEBUGFLAGS)
test: $(TEST_BIN)
	@echo "Running tests..."
	@$(TEST_BIN)

# Build test runner
$(TEST_BIN): $(TEST_OBJS) $(LIB_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Built test runner: $@"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test files
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile library files
$(BUILD_DIR)/lib_%.o: $(LIB_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile runtime files
$(BUILD_DIR)/runtime_%.o: $(RUNTIME_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(RUNTIME_DIR) -c $< -o $@

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "✓ Cleaned build artifacts"

# Install fern compiler
.PHONY: install
install: release
	install -m 755 $(FERN_BIN) /usr/local/bin/fern
	@echo "✓ Installed fern to /usr/local/bin/fern"

# Uninstall
.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/fern
	@echo "✓ Uninstalled fern"

# Run with Valgrind for memory checking
.PHONY: memcheck
memcheck: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(FERN_BIN)

# Format code
.PHONY: fmt
fmt:
	clang-format -i $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h $(TEST_DIR)/*.c

# Full quality check (build + test + style, strict mode)
.PHONY: check
check:
	@uv run scripts/check_style.py $(SRC_DIR) $(LIB_DIR)

# Style check only (no build/test)
.PHONY: style
style:
	@uv run scripts/check_style.py --style-only $(SRC_DIR) $(LIB_DIR)

# Lenient style check (warnings allowed)
.PHONY: style-lenient
style-lenient:
	@uv run scripts/check_style.py --style-only --lenient $(SRC_DIR) $(LIB_DIR)

# Pre-commit hook check
.PHONY: pre-commit
pre-commit:
	@uv run scripts/check_style.py --pre-commit $(SRC_DIR) $(LIB_DIR)

# Test examples - type check all .fn files in examples/
# Note: We only check compilation, not execution, since some examples
# return non-zero exit codes intentionally (e.g., hello.fn returns 42)
.PHONY: test-examples
test-examples: debug
	@echo "Testing examples..."
	@failed=0; \
	for f in examples/*.fn; do \
		name=$$(basename "$$f"); \
		printf "  %-30s" "$$name"; \
		if $(FERN_BIN) check "$$f" > /dev/null 2>&1; then \
			echo "✓ PASS"; \
		else \
			echo "✗ FAIL"; \
			$(FERN_BIN) check "$$f" 2>&1 | head -5; \
			failed=1; \
		fi; \
	done; \
	if [ $$failed -eq 1 ]; then \
		echo ""; \
		echo "Some examples failed type checking!"; \
		exit 1; \
	else \
		echo ""; \
		echo "✓ All examples type check!"; \
	fi

# Help
.PHONY: help
help:
	@echo "Fern Compiler - Build Targets"
	@echo ""
	@echo "  make              - Build debug version"
	@echo "  make debug        - Build debug version with symbols"
	@echo "  make release      - Build optimized release version"
	@echo "  make test         - Build and run all tests"
	@echo "  make test-examples- Type check and run all examples"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make install      - Install fern to /usr/local/bin"
	@echo "  make uninstall    - Remove installed fern"
	@echo "  make memcheck     - Run with Valgrind"
	@echo "  make fmt          - Format code with clang-format"
	@echo ""
	@echo "Quality Checks (strict mode - warnings are errors):"
	@echo "  make check        - Full check (build + test + style)"
	@echo "  make style        - FERN_STYLE only (strict)"
	@echo "  make style-lenient - FERN_STYLE only (warnings allowed)"
	@echo "  make pre-commit   - Pre-commit hook check"
	@echo ""
	@echo "  make help         - Show this help message"
