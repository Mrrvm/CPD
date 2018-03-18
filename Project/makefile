CC = gcc
CFLAGS = -fopenmp -march=native -std=gnu11 -Wall -Wextra -g -Wpedantic -Wshadow -Wstrict-overflow -fno-strict-aliasing
CFLAGS_RELEASE = -march=native -std=gnu11 -O2
BINARY = kuduro

all: client

default:	client

client: $(wildcard *.c)
	$(CC) $(wildcard *.c) $(CFLAGS) -o $(BINARY)

clean:
	-rm $(BINARY)
	-rm $(wildcard *.o)

.PHONY: default all clean test
