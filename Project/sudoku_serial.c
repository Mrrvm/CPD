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
void printGrid(int grid[N][N]) {
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

    fclose(sudoku_file);
    return 0;
}
