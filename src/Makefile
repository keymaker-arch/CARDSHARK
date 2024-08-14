CC := gcc
CFLAGS := -pthread -static

BIN_DIR := ./bin
LIB_SRC := $(shell find ./lib -name "lib*.c")
TARGET_T := $(LIB_SRC:./lib/lib_%.c=$(BIN_DIR)/%)

.PHONY: all clean

all: $(TARGET_T)

$(TARGET_T): $(BIN_DIR)/%: ./lib/lib_%.c interface/interface.c interface/interface.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I interface/ -o $@ $^

clean:
	@rm -rf $(BIN_DIR)