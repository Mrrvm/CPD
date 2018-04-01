CC = gcc
CFLAGS = -march=native -std=gnu11 -Wall -Wextra -Werror -g -Wpedantic -Wshadow -Wstrict-overflow -fno-strict-aliasing
CFLAGS_RELEASE = -march=native -std=gnu11 -O2 -Os
BINARY = kuduro

all: serial-recur serial-iter omp

default: omp

serial-recur: serial/sudoku-serial-recur.c
	$(CC) serial/sudoku-serial-recur.c $(CFLAGS) -o $(BINARY)-serial-recur

serial-iter: serial/sudoku-serial-iter.c
	$(CC) serial/sudoku-serial-iter.c $(CFLAGS) -o $(BINARY)-serial-iter

omp: omp/
	$(CC) omp/sudoku-ompv1.c -fopenmp $(CFLAGS) -o $(BINARY)-ompv1
	$(CC) omp/sudoku-ompv2.c -fopenmp $(CFLAGS) -o $(BINARY)-ompv2

clean:
	-@rm $(BINARY)*

.PHONY: default all clean test serial-recur serial-iter omp
