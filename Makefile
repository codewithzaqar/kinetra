CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c11 -Iinclude -MMD -MP
LDFLAGS = -lm

# Enable OpenMP: make OPENMP=1
ifeq ($(OPENMP),1)
CFLAGS += -fopenmp
LDFLAGS += -fopenmp
endif

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/vm.c $(SRC_DIR)/hpc_math.c $(SRC_DIR)/diagnostics.c $(SRC_DIR)/bytecode.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = kinetra

-include $(OBJS:.o=.d)

.PHONY: all clean run test

test: $(TARGET)
	powershell -NoProfile -ExecutionPolicy Bypass -File tests/run_tests.ps1

all: directories $(TARGET)

directories:
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Kinetra v0.0.1a01 built successfully."

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: all
	./$(TARGET) test.knt