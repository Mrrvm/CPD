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
    square *empty_sq;
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
            if (to_solve->grid[i + row_start][j + col_start] == num)
                return false;
    return true;
}

bool valid_in_col_and_row(int col, int row, int num) {
    for (int i = 0; i < to_solve->n; i++) {
        if (to_solve->grid[i][col] == num || to_solve->grid[row][i] == num) {
            return false;
        }
    }
    return true;
}

// Check if it is safe to put number in position
bool safe(int row, int col, int num) {
    return valid_in_box(row, col, num) && valid_in_col_and_row(col, row, num);
}

int solve() {
    uint8_t *plays = NULL;
    int ptr;
    int N = to_solve->n;
    int nplays = to_solve->n_plays;

    /* Array to keep history of previous plays (for backtracking) */
    /* One for each empty square of the initial sudoku */
    plays = (uint8_t *)malloc(nplays * sizeof(uint8_t));

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
        if (plays[ptr] > N) {
            if (ptr == 0) {
                /* No solution */
                return 0;
            } else {
                /* Backtrack */
                ptr--;

                to_solve
                ->grid[to_solve->empty_sq[ptr].row][to_solve->empty_sq[ptr].col] =
                    0;
                continue;
            }
        }

        /* Check if next play is valid. */
        if (safe(to_solve->empty_sq[ptr].row, to_solve->empty_sq[ptr].col,
                 plays[ptr])) {
            to_solve->grid[to_solve->empty_sq[ptr].row][to_solve->empty_sq[ptr].col] =
                plays[ptr];
            plays[ptr]++; /* always to next */

            ptr++;
            if (ptr < nplays) {
                /* Branch */

                plays[ptr] = 1;
            } else {
                /* Puzzle solved */

                return 1;
            }
        } else {
            /* Try again*/
            plays[ptr]++;
        }
    }
}

int read_file(const char *filename) {
    FILE *sudoku_file;
    int box_size;
    int iter;
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

    // Create aux array of empty squares
    to_solve->n_plays = iter;
    to_solve->empty_sq = (square *)malloc(iter * sizeof(square));

    iter = 0;
    for (int i = 0; i < to_solve->n; i++) {
        for (int j = 0; j < to_solve->n; j++) {
            if (to_solve->grid[i][j] == 0) {
                to_solve->empty_sq[iter].row = i;
                to_solve->empty_sq[iter].col = j;
                to_solve->empty_sq[iter].box =
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
    if (solve() == 1) {
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
