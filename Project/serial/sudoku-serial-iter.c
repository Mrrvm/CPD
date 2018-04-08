// Sudoku Solver
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_ARGS 2
#define EMPTY 0

typedef struct square_struct {
    int_fast32_t row;
    int_fast32_t col;
    int_fast32_t box;
} square;

typedef struct sudoku_struct {
    int_fast32_t n;
    int_fast32_t box_size;
    uint_fast8_t **grid;
    int_fast32_t n_plays;
    uint_fast8_t *plays;
    square **empty_sq;
} sudoku;

sudoku *to_solve;

void print_error(char *error) {
    fprintf(stderr, error);
    exit(EXIT_FAILURE);
}

void free_sudoku(sudoku *to_free) {
    for (int i = 0; i < to_free->n; i++) {
        free(to_free->grid[i]);
    }
    free(to_free);
}

sudoku *init_sudoku(int box_size) {
    sudoku *new_sudoku = (sudoku *)malloc(sizeof(sudoku));

    new_sudoku->box_size = box_size;
    new_sudoku->n = box_size * box_size;
    new_sudoku->n_plays = 0;
    new_sudoku->empty_sq = NULL;

    // Allocate matrix
    new_sudoku->grid = (uint8_t **)malloc(new_sudoku->n * sizeof(uint8_t *));
    if (new_sudoku->grid == NULL)
        print_error("Could not allocate space\n");
    for (int i = 0; i < new_sudoku->n; i++) {
        new_sudoku->grid[i] = (uint8_t *)malloc(new_sudoku->n * sizeof(uint8_t));
        if (new_sudoku->grid[i] == NULL)
            print_error("Could not allocate space\n");
    }
    return new_sudoku;
}

// Print grid
void print_grid() {
    for (int row = 0; row < to_solve->n; row++) {
        for (int col = 0; col < to_solve->n; col++)
            printf("%2d ", to_solve->grid[row][col]);
        printf("\n");
    }
}

// Check box
bool valid_in_box(int row, int col, int num) {
    uint8_t row_start = row - row % to_solve->box_size;
    uint8_t col_start = col - col % to_solve->box_size;

    for (int i = 0; i < to_solve->box_size; i++)
        for (int j = 0; j < to_solve->box_size; j++)
            if (to_solve->grid[i + row_start][j + col_start] == num) {
                return false;
            }
    return true;
}

bool valid_in_row_and_col(int row, int col, int num) {
    for (int i = 0; i < to_solve->n; i++) {
        if (to_solve->grid[i][col] == num || to_solve->grid[row][i] == num) {
            return false;
        }
    }
    return true;
}

// Check if it is safe to put number in position
bool safe(int row, int col, int num) {
    return valid_in_box(row, col, num) && valid_in_row_and_col(row, col, num);
}

bool safe_by_square(square *to_test, int num) {
    return valid_in_box(to_test->row, to_test->col, num) &&
           valid_in_row_and_col(to_test->row, to_test->col, num);
}

void set_by_ptr(int ptr, uint_fast8_t val) {
    to_solve->grid[to_solve->empty_sq[ptr]->row][to_solve->empty_sq[ptr]->col] =
        val;
}

square *get_square_by_ptr(int ptr) {
    return to_solve->empty_sq[ptr];
}

uint_fast8_t get_candidate(int ptr) {
    return to_solve->plays[ptr];
}

void set_candidate(int ptr, int val) {
    to_solve->plays[ptr] = val;
}

void increment_candidate(int ptr) {
    to_solve->plays[ptr]++;
}

int solve() {
    int ptr;

    /* Array to keep history of previous plays (for backtracking) */
    /* One for each empty square of the initial sudoku */
    uint8_t *plays = to_solve->plays;

    /* Next play to try. */
    ptr = 0;
    plays[0] = 1;

    while (1) {
        /* This square is empty. */
        /*
        printf("r: %ld ; c: %ld ; b: %ld ; play: %2" SCNu8 "\n",
          to_solve->empty_sq[ptr].row, to_solve->empty_sq[ptr].col,
          to_solve->empty_sq[ptr].box, plays[ptr]);
        */

        /* Check if branch options are emptied. */
        if (plays[ptr] > to_solve->n) {
            if (ptr == 0) {
                /* No solution */

                free(plays);
                return 0;
            }
            /* Backtrack */
            ptr--;

            set_by_ptr(ptr, 0);
            continue;
        }

        /* Check if next play is valid. */
        if (safe_by_square(get_square_by_ptr(ptr), get_candidate(ptr))) {
            set_by_ptr(ptr, get_candidate(ptr));
            plays[ptr]++; /* always to next */

            ptr++;
            if (ptr < to_solve->n_plays) {
                /* Branch */
                set_candidate(ptr, 1);
            } else {
                /* Puzzle solved */
                free(plays);
                return 1;
            }
        } else {
            /* Try again*/
            increment_candidate(ptr);
        }
    }
}

int read_file(const char *filename) {
    FILE *sudoku_file;
    int box_size;
    uint_fast32_t iter;
    uint8_t num;

    // Opens file
    sudoku_file = fopen(filename, "r");
    if (sudoku_file == NULL)
        print_error("Could not open file\n");

    // Scans first line (aka the square size)
    if (fscanf(sudoku_file, "%d", &box_size) == EOF)
        print_error("Could not read file\n");
    to_solve = init_sudoku(box_size);

    // Read the file
    iter = 0;
    for (int i = 0; i < to_solve->n; i++) {
        for (int j = 0; j < to_solve->n; j++) {
            fscanf(sudoku_file, "%2" SCNu8, &num);

            to_solve->grid[i][j] = num;
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
    to_solve->n_plays = iter;
    to_solve->empty_sq = (square **)malloc(iter * sizeof(square *));

    for (uint_fast32_t i = 0; i < iter; i++) {
        to_solve->empty_sq[i] = (square *)malloc(iter * sizeof(square));
    }

    to_solve->plays = (uint8_t *)malloc(to_solve->n_plays * sizeof(uint8_t));

    iter = 0;
    for (int i = 0; i < to_solve->n; i++) {
        for (int j = 0; j < to_solve->n; j++) {
            if (to_solve->grid[i][j] == 0) {
                to_solve->empty_sq[iter]->row = i;
                to_solve->empty_sq[iter]->col = j;
                to_solve->empty_sq[iter]->box =
                    (i / to_solve->box_size) * to_solve->box_size +
                    j / to_solve->box_size;

                iter++;
            }
        }
    }

    fclose(sudoku_file);
    return 0;
}

int main(int argc, char const *argv[]) {

    clock_t begin, end;

    if (argc < N_ARGS) {
        char error[64];
        sprintf(error, "Usage: %s filename\n", argv[0]);
        print_error(error);
    }

    if (read_file(argv[1]) != 0) {
        char error[64];
        sprintf(error, "Unable to read file %s\n", argv[1]);
        print_error(error);
    }

    print_grid();
    printf("\n");

    begin = clock();
    // Solve the puzzle
    if (solve()) {
        end = clock();
        print_grid();
        printf("Solved Sudoku\n");
        // Solution found
    } else {
        end = clock();
        printf("Did not solve Sudoku\n");
        // No solution
    };

    free_sudoku(to_solve);
    printf("%f\n", (double)(end - begin) / CLOCKS_PER_SEC);
    return 0;
}
