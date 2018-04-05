// vim:fdm=syntax foldlevel=5 tabstop=4 shiftwidth=4
/* Tasks
 * ----------------
 * The queue is composed of items, each one having a puzzle grid.
 * Each processor takes one item of the queue and distributes it by N threads
 * If the thread see the play as valid, inserts a new item with it in queue
 * Slower than serial.
 * In this version threads dont wait for each other.
 *
 * Benchmarks
 * 4x4: 0.0004s
 * 9x9: 50s
 * 16x16: 46.3s
 *
 * Tests
 * Tried a filled squares vector, to speedup the grid copy in node creation,
 * Tried parallelizing the copy
 *
 *
 * Notes
 * Using 5 cores.
 */

#include <errno.h>
#include <inttypes.h>
#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 2
#define N_ARGS 2

typedef uint_fast8_t *square;

typedef uint_fast8_t *sudoku;

// Mother of all sudokus
typedef struct moas_t {
  square empty_sqs;
  sudoku to_solve;
  int_fast32_t n;
  int_fast32_t box_size;
  int_fast32_t n_empty_sq;
} moas;

int gDONE;
moas *gMOAS;

void print_error(char *error) {
  fprintf(stderr, error);
  exit(EXIT_FAILURE);
}

int read_file(const char *filename) {
  FILE *sudoku_file;
  int box_size;
  uint8_t num;

  /* Opens file */
  sudoku_file = fopen(filename, "r");
  if (sudoku_file == NULL)
    print_error("Could not open file\n");

  /* Scans first line (aka the square size) */
  if (fscanf(sudoku_file, "%d", &box_size) == EOF)
    print_error("Could not read file\n");

  gMOAS = (moas *)malloc(sizeof(moas));
  if (gMOAS == NULL) {
    print_error("Unable to init MOAS");
  }

  gMOAS->box_size = box_size;
  gMOAS->n = box_size * box_size;

  gMOAS->to_solve = (sudoku)malloc(gMOAS->n * gMOAS->n * sizeof(uint_fast8_t));

  /* Read the file */
  uint_fast32_t iter = 0;
  for (int i = 0; i < gMOAS->n; i++) {
    for (int j = 0; j < gMOAS->n; j++) {
      fscanf(sudoku_file, "%2" SCNu8, &num);
      gMOAS->to_solve[i * gMOAS->n + j] = num;
      if (num == 0) {
        iter++;
      }
    }
  }

  if (iter == 0) {
    fclose(sudoku_file);
    return 1;
  }

  // Create aux array of empty squares
  gMOAS->n_empty_sq = iter;
  gMOAS->empty_sqs = (square)malloc(iter * sizeof(uint_fast8_t));

  for (int i = 0, itera = 0; i < gMOAS->n * gMOAS->n; i++) {
    if (gMOAS->to_solve[i] == 0) {
      gMOAS->empty_sqs[itera] = i;
      itera++;
    }
  }

  fclose(sudoku_file);
  return 0;
}

// Free sudoku typedef
void free_sudoku(sudoku to_free) { free(to_free); }

// Free global MOAS
void free_gMOAS() {
  free_sudoku(gMOAS->to_solve);
  free(gMOAS->empty_sqs);

  free(gMOAS);
}

// Print grid
void print_grid(sudoku to_print) {
  for (int i = 0; i < gMOAS->n; i++) {
    for (int j = 0; j < gMOAS->n; j++) {
      printf("%2d ", to_print[i * gMOAS->n + j]);
    }
    printf("\n");
  }
  printf("\n");
}

// Check box, row and column for safety
bool safe(sudoku to_check, int id, int num) {
  int x = id / gMOAS->n, y = id % gMOAS->n;

  int bx = (x / gMOAS->box_size) * gMOAS->box_size,
      by = (y / gMOAS->box_size) * gMOAS->box_size;
  int j, ox, oy;
  for (j = 0; j < gMOAS->n; j++) {
    ox = j / gMOAS->box_size;
    oy = j % gMOAS->box_size;
    // check row
    if (y != j && num == to_check[x * gMOAS->n + j])
      return false;
    // check column
    if (x != j && num == to_check[j * gMOAS->n + y])
      return false;
    // check box
    if (id != ((bx + ox) * gMOAS->n + by + oy) &&
        num == to_check[(bx + ox) * gMOAS->n + by + oy])
      return false;
  }
  return true;
}

void copy_to_gMOAS(sudoku solved) {
  for (int i = 0; i < gMOAS->n_empty_sq; i++) {
    gMOAS->to_solve[gMOAS->empty_sqs[i]] = solved[gMOAS->empty_sqs[i]];
  }
}

sudoku new_state_copy(sudoku old_state) {
  sudoku new_state = (sudoku)malloc(gMOAS->n * gMOAS->n * sizeof(uint_fast8_t));
  memcpy(new_state, old_state, gMOAS->n * gMOAS->n * sizeof(uint_fast8_t));
  return new_state;
}

void solve(int id, sudoku state) {
  if (gDONE) {
    return;
  }

  /* if in the last solve layer check if solved and start popping */
  if (id == gMOAS->n_empty_sq - 1) {
    for (int i = 1; i <= gMOAS->n; i++) {
      if (safe(state, gMOAS->empty_sqs[id], i)) {
        state[gMOAS->empty_sqs[id]] = i;
        copy_to_gMOAS(state);
        gDONE = true;
        return;
      }
    }
    free_sudoku(state);
    return;
  }

  /* try each son solution */
  for (int i = 1; i <= gMOAS->n; i++) {
#pragma omp task firstprivate(i, id, state)
    {
      if (safe(state, gMOAS->empty_sqs[id], i)) {
        sudoku new_state = new_state_copy(state);
        new_state[gMOAS->empty_sqs[id]] = i;
        solve(id + 1, new_state);
      }
    }
  }

#pragma omp taskwait
  free_sudoku(state);
}

int main(int argc, char const *argv[]) {

  if (argc != N_ARGS) {
    char error[64];
    sprintf(error, "Usage: %s filename\n", argv[0]);
    print_error(error);
  }

  if (read_file(argv[1]) != 0) {
    char error[64];
    sprintf(error, "Unable to read file %s\n", argv[1]);
    print_error(error);
  }

  puts("~~~ Input Sudoku ~~~");
  print_grid(gMOAS->to_solve);

  // Solve the puzzle
  sudoku new_state = new_state_copy(gMOAS->to_solve);
  omp_set_num_threads(THREADS);
  double start = omp_get_wtime();
#pragma omp parallel
  {
#pragma omp single
    solve(0, new_state);
  }
  double finish = omp_get_wtime();
  if (gDONE) {
    puts("~~~ Output Sudoku ~~~");
    print_grid(gMOAS->to_solve);
    printf("Solved Sudoku\n");
    // Solution found
  } else {
    printf("Did not solve Sudoku\n");
    // No solution
  };

  free_gMOAS();
  printf("Total Time %lfs\n", (double)(finish - start));
  return 0;
}
