// Sudoku Solver
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N_ARGS 2
#define EMPTY 0

int BOX_SIZE = 0;
int N = 0;

void print_error(char *error) {
    fprintf(stderr, error);
    exit(EXIT_FAILURE);
}

// Print grid
void print_grid(int **grid) {
    for (int row = 0; row < N; row++) {
        for (int col = 0; col < N; col++)
            printf("%2d", grid[row][col]);
        printf("\n");
    }
}

// Check box
bool valid_in_box(int **grid, int row, int col, int num) {
    uint8_t row_start = row - row % BOX_SIZE;
    uint8_t col_start = col - col % BOX_SIZE;

    for (int i = 0; i < BOX_SIZE; i++)
        for (int j = 0; j < BOX_SIZE; j++)
            if (grid[i + row_start][j + col_start] == num)
                return false;
    return true;
}

bool valid_in_row(int **grid, int row, int num) {
    for (int j = 0; j < N; j++)
        if (grid[row][j] == num)
            return false;
    return true;
}

bool valid_in_col(int **grid, int col, int num) {
    for (int i = 0; i < N; i++)
        if (grid[i][col] == num)
            return false;
    return true;
}

// Check if it is safe to put number in position
bool safe(int **grid, int row, int col, int num) {
    return valid_in_box(grid, row, col, num) && valid_in_col(grid, col, num) &&
           valid_in_row(grid, row, num);
}

int last_square(int row, int col) {
    return ((row) == (N - 1) && (col) == (N - 1));
}

int nxt_row(int row, int col) {
    return ((col) < (N - 1)) ? (row) : (row + 1);
}

int nxt_col(int col) {
    return ((col) < (N - 1)) ? (col + 1) : 0;
}

int solve(int **grid, int row, int col) {
    int num;

    if (grid[row][col] != 0) {
        /* This square had an initial number. */

        if (last_square(row, col)) {
            /* Puzzle solved (leaf). */

            return 1;
        } else if (solve(grid, nxt_row(row, col), nxt_col(col)) == 1) {
            /* Puzzle solved (branch). */

            return 1;
        }
    } else {
        /* This square is empty. */

        /* Try all valid solutions. */
        for (num = 1; num <= 9; num++) {
            if (safe(grid, row, col, num)) {
                grid[row][col] = num; /* Tries this number. */

                if (last_square(row, col)) {
                    /* Puzzle solved (leaf). */

                    return 1;
                } else if (solve(grid, nxt_row(row, col), nxt_col(col)) == 1) {
                    /* Puzzle solved (branch). */

                    return 1;
                }

                grid[row][col] = 0; /* Deletes change. */
            }
        }
    }

    /* There is no solution for this puzzle. */
    return 0;
}

int main(int argc, char const *argv[]) {

    FILE *sudoku_file;
    int **matrix = NULL;

    if (argc != N_ARGS) {
        char error[64];
        sprintf(error, "Usage: %s filename\n", argv[0]);
        print_error(error);
    }

    // Opens file
    sudoku_file = fopen(argv[1], "r");
    if (sudoku_file == NULL)
        print_error("Could not open file\n");

    // Scans first line (aka the square size)
    if (fscanf(sudoku_file, "%d", &BOX_SIZE) == EOF)
        print_error("Could not read file\n");
    N = BOX_SIZE * BOX_SIZE;

    // Allocate matrix
    matrix = (int **)malloc(N * sizeof(int *));
    if (matrix == NULL)
        print_error("Could not allocate space\n");
    for (int i = 0; i < N; i++) {
        matrix[i] = (int *)malloc(N * sizeof(int));
        if (matrix[i] == NULL)
            print_error("Could not allocate space\n");
    }

    // Read the file
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fscanf(sudoku_file, "%d", *(matrix + i) + j);
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }

    // Solve the puzzle
    if (solve(matrix, 0, 0) == 1) {
        print_grid(matrix);
        printf("Solved Sudoku\n");
        // Solution found
    } else {
        printf("Did not solve Sudoku\n");
        // No solution
    };

    fclose(sudoku_file);
    return 0;
}
