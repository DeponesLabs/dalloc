# Compiler
CC = gcc

# Flags:
# -g    : add Debug Symbols (necessary for GDB)
# -Wall : warn all
CFLAGS = -g -Wall -Wextra -pthread -Wno-misleading-indentation

# Virtual Targets (Prevents file name conflicts)
.PHONY: all clean

# Rule
all: dalloc hack_demo thread_test

# -----------------------------------------------------------
# 1. Main Program (Unit Tests - Stage 1, 2, 3, 4)
# -----------------------------------------------------------
dalloc: main.o dalloc.o
	$(CC) $(CFLAGS) -o dalloc main.o dalloc.o

# -----------------------------------------------------------
# 2. Hack Demo (Security Test)
# -----------------------------------------------------------
hack_demo: hack_demo.o dalloc.o dstring.o
	$(CC) $(CFLAGS) -o hack_demo hack_demo.o dalloc.o dstring.o

# -----------------------------------------------------------
# 3. Thread Test (Stress Test)
# -----------------------------------------------------------
thread_test: thread_test.o dalloc.o
	$(CC) $(CFLAGS) -o thread_test thread_test.o dalloc.o

# -----------------------------------------------------------
# Obj Files (.o) - They are only compiled when they change.
# -----------------------------------------------------------

main.o: main.c dalloc.h
	$(CC) $(CFLAGS) -c main.c

hack_demo.o: hack_demo.c dalloc.h dstring.h
	$(CC) $(CFLAGS) -c hack_demo.c

thread_test.o: thread_test.c dalloc.h
	$(CC) $(CFLAGS) -c thread_test.c

dalloc.o: dalloc.c dalloc.h
	$(CC) $(CFLAGS) -c dalloc.c

dstring.o: dstring.c dstring.h
	$(CC) $(CFLAGS) -c dstring.c

# -----------------------------------------------------------
# TEMİZLİK
# -----------------------------------------------------------
clean:
	rm -f *.o dalloc hack_demo thread_test
