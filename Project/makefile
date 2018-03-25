CC = gcc
CFLAGS = -march=native -std=gnu11 -Wall -Wextra -Werror -g -Wpedantic -Wshadow -Wstrict-overflow -fno-strict-aliasing
CFLAGS_RELEASE = -march=native -std=gnu11 -O2 -Os
BINARY = kuduro

all: serial omp

default: omp

serial: sudoku-serial.c
	$(CC) sudoku-serial.c $(CFLAGS) -o $(BINARY)-serial

omp: sudoku-omp.c
	$(CC) sudoku-omp.c -fopenmp $(CFLAGS) -o $(BINARY)-omp

clean:
	-rm $(BINARY)*
	-rm $(wildcard *.o)

.PHONY: default all clean test serial omp
