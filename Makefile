########################################
# BUILD MODE
########################################
# dev     - Standard dev build
# debug   - Enables debug logs
# release - Builds with all optimizations
BUILD_MODE ?= dev

CFLAGS_EXT =

ifeq ($(BUILD_MODE),dev)

CFLAGS_EXT += -fanalyzer          # GCC static analyzer 
CFLAGS_EXT += -Wshadow            # Warn when variables shadow others
CFLAGS_EXT += -Wpointer-arith     # Warn on void*/function pointer arithmetic
CFLAGS_EXT += -Wcast-align        # Warn about misaligned pointer casts
CFLAGS_EXT += -Wstrict-prototypes # Enforce proper function prototypes
CFLAGS_EXT += -Wcast-qual         # Warn when casts discard const/volatile
CFLAGS_EXT += -Wswitch-default    # Warn if switch has no default case
CFLAGS_EXT += -Wswitch-enum       # Warn if not all enum values handled
CFLAGS_EXT += -Wconversion        # Warn on implicit type conversions
CFLAGS_EXT += -Waggregate-return  # Warn when functions return structs/unions

else ifeq ($(BUILD_MODE),debug)
CFLAGS_EXT +=

else ifeq ($(BUILD_MODE),release)
CFLAGS_EXT += -O2 -DNDEBUG

endif

# Include timestamp POSIX
CFLAGS_EXT += -D_POSIX_C_SOURCE=200809L

########################################
# PATHS
########################################
SRC_DIR = src
BIN_DIR = bin


########################################
# FILES
########################################
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRCS))

DEPS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.d, $(SRCS))
TARGET = $(BIN_DIR)/c_server


########################################
# COMPILER + FLAGS
########################################
INC_FLAGS = -I$(SRC_DIR)/include

CC = gcc
CFLAGS = -std=c99 $(INC_FLAGS) -Wall -Wextra $(CFLAGS_EXT)


########################################
# RULES
########################################

# Target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Objects
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)


########################################
# SETUP
########################################
.PHONY: setup clean

setup:
	@mkdir -p $(BIN_DIR)

clean:
	@rm -rf bin


########################################
# BUILD DEV
########################################
.PHONY: build run

build: setup $(TARGET)

run: setup $(TARGET)
	$(TARGET)