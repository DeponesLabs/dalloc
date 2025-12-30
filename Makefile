# Compiler
CC = gcc

# Flags:
# -g    : add Debug Symbols (necessary for GDB)
# -Wall : warn all
CFLAGS = -g -Wall

# Output filename
TARGET = dalloc

# Source codes
SRCS = test.c dalloc.c

# Rule
all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
