CC = gcc
CFLAGS = -march=native -std=gnu11 -Wall -Wextra -Werror -g -Wpedantic -Wshadow -Wstrict-overflow -fno-strict-aliasing
CFLAGS_OPTIM = -march=native -std=gnu11 -O3 -Os
CFLAGS_RELEASE = -march=native -std=gnu11 -O2 -Os
BINARY = kuduro

all: serial-recur serial-iter omp

default: omp

serial: serial/sudoku-serial.c
	$(CC) serial/sudoku-serial.c $(CFLAGS) -o $(BINARY)-serial


BPsubqueues: mpi/sudoku.c
	$(CC) omp/sudoku-omp_BPsubqueues.c -fopenmp $(CFLAGS_OPTIM) $(CFLAGS) -o $(BINARY)-omp

clean:
	-@rm $(BINARY)*

.PHONY: default all clean test mpi serial
