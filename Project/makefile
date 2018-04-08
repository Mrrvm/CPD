CC = gcc
CFLAGS = -march=native -std=gnu11 -Wall -Wextra -Werror -g -Wpedantic -Wshadow -Wstrict-overflow -fno-strict-aliasing
CFLAGS_OPTIM = -march=native -std=gnu11 -O3 -Os
CFLAGS_RELEASE = -march=native -std=gnu11 -O2 -Os
BINARY = kuduro

all: serial-recur serial-iter omp

default: omp

serial-recur: serial/sudoku-serial-recur.c
	$(CC) serial/sudoku-serial-recur.c $(CFLAGS) -o $(BINARY)-serial

serial-iter: serial/sudoku-serial-iter.c
	$(CC) serial/sudoku-serial-iter.c $(CFLAGS) -o $(BINARY)-serial

tasks: omp/sudoku-omp_tasks.c
	$(CC) omp/sudoku-omp_tasks.c -fopenmp $(CFLAGS_OPTIM) $(CFLAGS) -o $(BINARY)-omp

reborn: omp/sudoku-omp-reborn.c
	$(CC) omp/sudoku-omp-reborn.c -fopenmp $(CFLAGS_OPTIM) $(CFLAGS) -o $(BINARY)-omp

adapta: omp/sudoku-omp-adapta.c
	$(CC) omp/sudoku-omp-adapta.c -fopenmp $(CFLAGS_OPTIM) $(CFLAGS) -o $(BINARY)-omp

masks: omp/sudoku-omp_masks.c
	$(CC) omp/sudoku-omp_masks.c -fopenmp $(CFLAGS_OPTIM) $(CFLAGS) -o $(BINARY)-omp

static: omp/sudoku-omp_tasks_stati.c
	$(CC) omp/sudoku-omp_tasks_stati.c -fopenmp $(CFLAGS_OPTIM) $(CFLAGS) -o $(BINARY)-omp

profile: omp/sudoku-omp_tasks.c
	$(CC) omp/sudoku-omp_tasks.c -fopenmp $(CFLAGS) -lprofiler -ltcmalloc -o $(BINARY)-omp

BPsubqueues: omp/sudoku-omp_BPsubqueues.c
	$(CC) omp/sudoku-omp_BPsubqueues.c -fopenmp $(CFLAGS_OPTIM) $(CFLAGS) -o $(BINARY)-omp

comparisson: omp/
	$(CC) omp/sudoku-omp-adapta.c -fopenmp $(CFLAGS_OPTIM) -o $(BINARY)-adapta
	$(CC) omp/sudoku-omp_bfs.c -fopenmp $(CFLAGS_OPTIM) -o $(BINARY)-bfs
	$(CC) omp/sudoku-omp_BPsubqueues.c -fopenmp $(CFLAGS_OPTIM) -o $(BINARY)-subqueues
	$(CC) omp/sudoku-omp_tasks.c -fopenmp $(CFLAGS_OPTIM) -o $(BINARY)-tasks
	$(CC) omp/sudoku-omp-reborn.c -fopenmp $(CFLAGS_OPTIM) -o $(BINARY)-reborn
	$(CC) serial/sudoku-serial-iter.c -fopenmp $(CFLAGS_OPTIM) -o $(BINARY)-serial

clean:
	-@rm $(BINARY)*

.PHONY: default all clean test serial-recur serial-iter omp
