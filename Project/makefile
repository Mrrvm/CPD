CC = gcc
CFLAGS = -march=native -std=gnu11 -Wall -Wextra -Werror -g -Wpedantic -Wshadow -Wstrict-overflow -fno-strict-aliasing
CFLAGS_OPTIM = -O2 -Os
CFLAGS_RELEASE = -march=native -std=gnu11 -O2 -Os
BINARY = kuduro

all: serial-recur serial-iter omp

default: omp

serial-recur: serial/sudoku-serial-recur.c
	$(CC) serial/sudoku-serial-recur.c $(CFLAGS) -o $(BINARY)-serial-recur

serial-iter: serial/sudoku-serial-iter.c
	$(CC) serial/sudoku-serial-iter.c $(CFLAGS) -o $(BINARY)-serial-iter

tasks: omp/sudoku-omp_tasks.c
	$(CC) omp/sudoku-omp_tasks.c -fopenmp $(CFLAGS_OPTIM) $(CFLAGS) -o $(BINARY)-omp_tasks

profile: omp/sudoku-omp_tasks.c
	$(CC) omp/sudoku-omp_tasks.c -fopenmp $(CFLAGS) -lprofiler -ltcmalloc -o $(BINARY)-profile

omp: omp/
	$(CC) omp/sudoku-omp_dfsv1.c -fopenmp $(CFLAGS) -o $(BINARY)-omp_dfsv1
	$(CC) omp/sudoku-omp_dfsv2.c -fopenmp $(CFLAGS) -o $(BINARY)-omp_dfsv2
	$(CC) omp/sudoku-omp_dfsv3.c -fopenmp $(CFLAGS) -o $(BINARY)-omp_dfsv3
	$(CC) omp/sudoku-omp_tasks.c -fopenmp $(CFLAGS) -o $(BINARY)-omp_tasks
	$(CC) omp/sudoku-omp_subqueues.c -fopenmp $(CFLAGS) -o $(BINARY)-omp_subqueries

clean:
	-@rm $(BINARY)*

.PHONY: default all clean test serial-recur serial-iter omp
