# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99

# Source files
SRCS = main.c arg_parser.c command_parser.c tcp_client.c udp_client.c

# Object files
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = ipk24chat-client

# Default target
all: $(TARGET) clean

# Linking object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compiling C source files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up
clean:
	rm -f $(OBJS)
