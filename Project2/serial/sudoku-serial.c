// vim:tabstop=4 shiftwidth=4
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

typedef struct moas {
    int n;
    int box_size;
    int **grid;
    int **known;
    int *rows;
    int *cols;
    int *squares;
    int *bits;
} moas_t;

moas_t *gMOAS;
bool gDONE;

int square(int i, int j) {
    return (i / gMOAS->box_size) * gMOAS->box_size + j / gMOAS->box_size;
}

void set_cell(int i, int j, int n) {
    gMOAS->grid[i][j] = n;
    gMOAS->rows[i] |= gMOAS->bits[n];
    gMOAS->cols[j] |= gMOAS->bits[n];
    gMOAS->squares[square(i, j)] |= gMOAS->bits[n];
}

int clear_cell(int i, int j) {
    int n = gMOAS->grid[i][j];
    gMOAS->grid[i][j] = 0;
    gMOAS->rows[i] &= ~gMOAS->bits[n];
    gMOAS->cols[j] &= ~gMOAS->bits[n];
    gMOAS->squares[square(i, j)] &= ~gMOAS->bits[n];
    return n;
}

void print_grid() {
    for (int x = 0; x < gMOAS->n; x++) {
        for (int y = 0; y < gMOAS->n; y++) {
            if (gMOAS->known[x][y]) {
                printf("%2d ", gMOAS->known[x][y]);
            } else {
                printf("%2d ", gMOAS->grid[x][y]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

bool is_available(int i, int j, int n) {
    return (gMOAS->rows[i] & gMOAS->bits[n]) == 0 &&
           (gMOAS->cols[j] & gMOAS->bits[n]) == 0 &&
           (gMOAS->squares[square(i, j)] & gMOAS->bits[n]) == 0;
}

bool advance_cell(int i, int j) {
    int n = clear_cell(i, j);
    while (++n <= gMOAS->n) {
        if (is_available(i, j, n)) {
            set_cell(i, j, n);
            return true;
        }
    }
    return false;
}

void solve() {
    int pos = 0;
    int total = gMOAS->n * gMOAS->n;
    while (1) {
        while (pos < total && gMOAS->known[pos / gMOAS->n][pos % gMOAS->n]) {
            ++pos;
        }
        if (pos >= total) {
            gDONE = true;
            break;
        }
        if (advance_cell(pos / gMOAS->n, pos % gMOAS->n)) {
            ++pos;
        } else {
            do {
                --pos;
            } while (pos >= 0 && gMOAS->known[pos / gMOAS->n][pos % gMOAS->n]);
            if (pos < 0) {
                break;
            }
        }
    }
}

int read_file(const char *filename) {
    FILE *sudoku_file;
    int box_size;
    uint8_t num;

    /* Opens file */
    sudoku_file = fopen(filename, "re");
    if (sudoku_file == NULL) {
        fprintf(stderr, "Could not open file\n");
        exit(2);
    }

    /* Scans first line (aka the square size) */
    if (fscanf(sudoku_file, "%d", &box_size) == EOF) {
        fprintf(stderr, "Could not read file\n");
        exit(2);
    }

    gMOAS = (moas_t *)malloc(sizeof(moas_t));
    if (gMOAS == NULL) {
        fprintf(stderr, "Unable to init MOAS");
        exit(2);
    }

    gMOAS->box_size = box_size;
    gMOAS->n = box_size * box_size;
    /* Init our bit mask */
    gMOAS->bits = (int *)calloc(gMOAS->n + 1, sizeof(int));
    for (int n = 1; n < gMOAS->n + 1; n++) {
        gMOAS->bits[n] = 1 << n;
    }

    /* Read the file */
    gMOAS->known = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->grid = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->rows = (int *)calloc(gMOAS->n, sizeof(int));
    gMOAS->cols = (int *)calloc(gMOAS->n, sizeof(int));
    gMOAS->squares = (int *)calloc(gMOAS->n, sizeof(int));
    for (int x = 0; x < gMOAS->n; x++) {
        gMOAS->known[x] = (int *)calloc(gMOAS->n, sizeof(int));
        gMOAS->grid[x] = (int *)calloc(gMOAS->n, sizeof(int));
        for (int y = 0; y < gMOAS->n; y++) {
            fscanf(sudoku_file, "%2" SCNu8, &num);
            set_cell(x, y, num);
            gMOAS->known[x][y] = num;
        }
    }

    fclose(sudoku_file);
    return 0;
}

void free_gMOAS() {
    for (int x = 0; x < gMOAS->n; x++) {
        free(gMOAS->known[x]);
        free(gMOAS->grid[x]);
    }

    free(gMOAS->known);
    free(gMOAS->grid);
    free(gMOAS->rows);
    free(gMOAS->cols);
    free(gMOAS->squares);
    free(gMOAS);
}

int main(int argc, char const *argv[]) {
    if (argc < N_ARGS) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        exit(1);
    }

    if (read_file(argv[1]) != 0) {
        fprintf(stderr, "Unable to read file %s\n", argv[1]);
        exit(1);
    }

    print_grid();

    // Solve the puzzle
    solve();

    if (gDONE == true) {
        print_grid();
        printf("Solved Sudoku\n");
        // Solution found
    } else {
        printf("Did not solve Sudoku\n");
        // No solution
    };

    free_gMOAS();
    return 0;
}
