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

typedef struct sudoku_struct {
    int_fast32_t n;
    int_fast32_t box_size;
    uint_fast8_t **grid;
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
            if (to_solve->grid[i + row_start][j + col_start] == num)
                return false;
    return true;
}

bool valid_in_row(int row, int num) {
    for (int j = 0; j < to_solve->n; j++)
        if (to_solve->grid[row][j] == num)
            return false;
    return true;
}

bool valid_in_col(int col, int num) {
    for (int i = 0; i < to_solve->n; i++)
        if (to_solve->grid[i][col] == num)
            return false;
    return true;
}

// Check if it is safe to put number in position
bool safe(int row, int col, int num) {
    return valid_in_box(row, col, num) && valid_in_col(col, num) &&
           valid_in_row(row, num);
}

int last_square(int row, int col) {
    return ((row) == (to_solve->n - 1) && (col) == (to_solve->n - 1));
}

int nxt_row(int row, int col) {
    return ((col) < (to_solve->n - 1)) ? (row) : (row + 1);
}

int nxt_col(int col) {
    return ((col) < (to_solve->n - 1)) ? (col + 1) : 0;
}

int solve(int row, int col) {
    int num;

    if (to_solve->grid[row][col] != 0) {
        /* This square had an initial number. */

        if (last_square(row, col)) {
            /* Puzzle solved (leaf). */

            return 1;
        } else if (solve(nxt_row(row, col), nxt_col(col)) == 1) {
            /* Puzzle solved (branch). */

            return 1;
        }
    } else {
        /* This square is empty. */

        /* Try all valid solutions. */
        for (num = 1; num <= to_solve->n; num++) {
            if (safe(row, col, num)) {
                to_solve->grid[row][col] = num; /* Tries this number. */

                if (last_square(row, col)) {
                    /* Puzzle solved (leaf). */

                    return 1;
                } else if (solve(nxt_row(row, col), nxt_col(col)) == 1) {
                    /* Puzzle solved (branch). */

                    return 1;
                }

                to_solve->grid[row][col] = 0; /* Deletes change. */
            }
        }
    }

    /* There is no solution for this puzzle. */
    return 0;
}

int read_file(const char *filename) {
    FILE *sudoku_file;
    int box_size;

    // Opens file
    sudoku_file = fopen(filename, "r");
    if (sudoku_file == NULL)
        print_error("Could not open file\n");

    // Scans first line (aka the square size)
    if (fscanf(sudoku_file, "%d", &box_size) == EOF)
        print_error("Could not read file\n");
    to_solve = init_sudoku(box_size);

    // Read the file
    for (int i = 0; i < to_solve->n; i++) {
        for (int j = 0; j < to_solve->n; j++) {
            fscanf(sudoku_file, "%2" SCNu8, *(to_solve->grid + i) + j);
        }
    }

    fclose(sudoku_file);
    return 0;
}

int main(int argc, char const *argv[]) {

    clock_t begin, end;

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

    print_grid();
    printf("\n");

    begin = clock();
    // Solve the puzzle
    if (solve(0, 0) == 1) {
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
    printf("Total Time %f\n", (double)(end - begin) / CLOCKS_PER_SEC);
    return 0;
}
