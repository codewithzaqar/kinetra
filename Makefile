CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c11 -Iinclude -MMD -MP
LDFLAGS = -lm

# Enable OpenMP: make OPENMP=1
ifeq ($(OPENMP),1)
CFLAGS += -fopenmp
LDFLAGS += -fopenmp
endif

PREFIX ?= /usr/local

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/vm.c \
	$(SRC_DIR)/hpc_math.c $(SRC_DIR)/diagnostics.c $(SRC_DIR)/bytecode.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)
TARGET = kinetra

.PHONY: all clean run test

all: $(TARGET)

# Directory rule; order-only prerequisite (| below) guarantees it exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Kinetra built successfully."

# | $(BUILD_DIR) = create dir first, but don't rebuild objects when dir mtime changes
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: all
	./$(TARGET) test.knt

# PowerShell host: pwsh on Linux/macOS, powershell on Windows
ifeq ($(OS),Windows_NT)
PSHELL = powershell
else
PSHELL = $(shell command -v pwsh 2>/dev/null || echo pwsh)
endif

test: $(TARGET)
	$(PSHELL) -NoProfile -ExecutionPolicy Bypass -File tests/run_tests.ps1

install: all
	mkdir -p $(PREFIX)/bin
	cp $(TARGET) $(PREFIX)/bin/
	@echo "Installed $(TARGET) to $(PREFIX)/bin/"

# Auto-generated header dependencies (from -MMD -MP)
-include $(DEPS)