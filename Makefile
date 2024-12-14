# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-deprecated-declarations
LDFLAGS = -lssl -lcrypto

# Source files and headers
SRCS = main.c backup.c deduplication.c
OBJS = $(SRCS:.c=.o)
HEADERS = backup.h deduplication.h

# Output executable
TARGET = program

# Default target
all: $(TARGET)

# Link the object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)

depend:
	$(CC) -MM $(SRCS) > dependencies.mk

-include dependencies.mk