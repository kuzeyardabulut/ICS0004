CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -Wunused-function -Wunused-variable -Wunused-parameter -Wunused-label -Wunused-result
LDFLAGS ?=

# Binary target path (produced under build/)
TARGET_NAME := exchange_store_cp1
TARGET := build/$(TARGET_NAME)

SRCS := main.c utils.c
OBJDIR := build
OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

run: all
	./$(TARGET)

test: all
	@echo "Running tests..."
	@./tests/test_runner.sh || echo "Tests exited with non-zero status"

clean:
	rm -rf $(OBJDIR)/*

.PHONY: all run test install clean help

help:
	@echo "Available targets:"
	@echo "  make         Build the project (default)"
	@echo "  make run     Build then run the program"
	@echo "  make test    Build then run tests/test_runner.sh"
	@echo "  make clean   Remove build artifacts"
