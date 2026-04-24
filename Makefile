CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -I./include
TARGET = minigit.exe

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/repo.c \
       $(SRC_DIR)/hash.c \
       $(SRC_DIR)/blob.c \
       $(SRC_DIR)/tree.c \
       $(SRC_DIR)/commit.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean

# тесты

TEST_TARGET = test_minigit.exe
TEST_SRC    = tests/test_minigit.c

TEST_OBJS   := $(filter-out $(OBJ_DIR)/main.o,$(OBJS))

test: $(TEST_TARGET)
	@echo "\nRunning tests...\n"
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(TEST_OBJS)
	@mkdir -p tests
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(TEST_OBJS)

test-clean:
	rm -f $(TEST_TARGET)
	rm -rf .minigit

.PHONY: test test-clean