CC = gcc
DISTCC = mpicc
CFLAGS = -march=native -std=gnu11 -Wall -Wextra -Werror -g -Wpedantic -Wshadow -Wstrict-overflow -fno-strict-aliasing
CFLAGS_RELEASE = -march=native -std=gnu11 -O2 -Os
CFLAGS_DEBUG = -g -march=native -std=gnu11 -ffast-math
BINARY = kuduro

all: serial mpi

default: mpi

serial: serial/sudoku-serial.c
	$(CC) serial/sudoku-serial.c $(CFLAGS_RELEASE) -o $(BINARY)-serial

mpi: mpi/main.c
	$(DISTCC) mpi/main.c $(CFLAGS_DEBUG) -o $(BINARY)-mpi

clean:
	-@rm $(BINARY)*

.PHONY: default all clean test mpi serial
