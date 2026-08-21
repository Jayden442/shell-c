# 1. Compiler and Flags
CC       := gcc
CFLAGS   := -Wall -Wextra -g -MMD -MP
TARGET   := my_program

# 2. Directory Paths
SRC_DIR  := ./src

# 3. Automatic File Detection in repo/src
# Finds all .c files inside repo/src
SRCS     := $(wildcard $(SRC_DIR)/*.c)
# Replaces .c extension with .o for object files in the same directory
OBJS     := $(SRCS:.c=.o)
# Tracks header dependencies (.d files) in the same directory
DEPS     := $(SRCS:.c=.d)

# 4. Build Rules
.PHONY: all clean

# Default rule
all: $(TARGET)

# Link step (creates the executable in the root directory)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lreadline

# Compilation step for files inside repo/src
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 5. Include Generated Dependencies
-include $(DEPS)

# 6. Cleanup
clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)
