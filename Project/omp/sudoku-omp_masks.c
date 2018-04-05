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
int thread_count = 4;
moas *gMOAS;

int read_file(const char *filename) {
    FILE *sudoku_file;
    int box_size;
    uint8_t num;

    /* Opens file */
    sudoku_file = fopen(filename, "re");
    if (sudoku_file == NULL) {
        perror("Could not open file\n");
        return -1;
    }

    /* Scans first line (aka the square size) */
    if (fscanf(sudoku_file, "%d", &box_size) == EOF) {
        perror("Could not read file\n");
        fclose(sudoku_file);
        return -1;
    }

    gMOAS = (moas *)malloc(sizeof(moas));
    if (gMOAS == NULL) {
        perror("Unable to init MOAS");
        fclose(sudoku_file);
        return -1;
    }

    gMOAS->box_size = box_size;
    gMOAS->n = box_size * box_size;

    gMOAS->to_solve = (sudoku)calloc(gMOAS->n * gMOAS->n, sizeof(uint_fast8_t));

    /* Read the file */
    uint_fast32_t iter = 0;
    for (int i = 0; i < gMOAS->n * gMOAS->n; i++) {
        fscanf(sudoku_file, "%2" SCNu8, &num);
        gMOAS->to_solve[i] = num;
        if (num == 0) {
            iter++;
        }
    }

    if (iter == 0) {
        fclose(sudoku_file);
        return 1;
    }

    // Create aux array of empty squares
    gMOAS->n_empty_sq = iter;
    gMOAS->empty_sqs = (square)calloc(iter, sizeof(uint_fast8_t));

    for (int i = 0, itera = 0; i < gMOAS->n * gMOAS->n; i++) {
        if (gMOAS->to_solve[i] == 0) {
            gMOAS->empty_sqs[itera] = i;
            itera++;
        }
    }

    fclose(sudoku_file);
    return 0;
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
bool safe(uint_fast8_t *mask, int id, int num) {
    int x = gMOAS->empty_sqs[id] / gMOAS->n, y = gMOAS->empty_sqs[id] % gMOAS->n;
    int bx = (x / gMOAS->box_size) * gMOAS->box_size,
        by = (y / gMOAS->box_size) * gMOAS->box_size;

    for (int i = 0; i < gMOAS->n_empty_sq; i++) {
        if (i == id) {
            continue;
        }

        int xi = gMOAS->empty_sqs[i] / gMOAS->n;
        int yi = gMOAS->empty_sqs[i] % gMOAS->n;

        // check row
        if (x == xi && num == mask[i])
            return false;
        // check column
        if (y == yi && num == mask[i])
            return false;
        // check box
        int bxi = (xi / gMOAS->box_size) * gMOAS->box_size,
            byi = (yi / gMOAS->box_size) * gMOAS->box_size;
        if (bx == bxi && by == byi && num == mask[i])
            return false;
    }

    for (int j = 0; j < gMOAS->n; j++) {
        // check row
        if (y != j && num == gMOAS->to_solve[x * gMOAS->n + j])
            return false;
        // check column
        if (x != j && num == gMOAS->to_solve[j * gMOAS->n + y])
            return false;
        // check box
        int ox = j / gMOAS->box_size;
        int oy = j % gMOAS->box_size;
        if (gMOAS->empty_sqs[id] != ((bx + ox) * gMOAS->n + by + oy) &&
                num == gMOAS->to_solve[(bx + ox) * gMOAS->n + by + oy])
            return false;
    }

    return true;
}

void print_grid_with_mask(uint_fast8_t *mask) {
    int iter = 0;
    for (int i = 0; i < gMOAS->n; i++) {
        for (int j = 0; j < gMOAS->n; j++) {
            if (gMOAS->to_solve[i * gMOAS->n + j] == 0) {
                printf("%2d ", mask[iter]);
                iter++;
            } else {
                printf("%2d ", gMOAS->to_solve[i * gMOAS->n + j]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

void print_mask(uint_fast8_t *mask) {
    printf("Mask: ");
    for (int i = 0; i < gMOAS->n_empty_sq; i++) {
        printf("%d ", mask[i]);
    }
    printf("\n");
}

void solve(int id, uint_fast8_t *mask) {
    /* print_mask(c_Hack.mask); */
    for (int i = 1; i <= gMOAS->n; i++) {
        #pragma omp task firstprivate(i, id, mask) untied
        {
            if (safe(mask, id, i)) {
                uint_fast8_t *new_mask =
                malloc(gMOAS->n_empty_sq * sizeof(uint_fast8_t));
                memcpy(new_mask, mask, gMOAS->n_empty_sq * sizeof(uint_fast8_t));
                new_mask[id] = i;
                if (id == gMOAS->n_empty_sq - 1) {
                    for (int j = 0; j < gMOAS->n_empty_sq; j++) {
                        gMOAS->to_solve[gMOAS->empty_sqs[j]] = new_mask[j];
                    }
                    gDONE = true;
                } else {
                    solve(id + 1, new_mask);
                }
                free(new_mask);
            }
        }
    }
    #pragma omp taskwait
}

int main(int argc, char const *argv[]) {
    if (argc < N_ARGS) {
        char error[64];
        sprintf(error, "Usage: %s filename\n", argv[0]);
        perror(error);
        return 1;
    } else if (argc == 3) {
        thread_count = atoi(argv[2]);
    }

    if (read_file(argv[1]) != 0) {
        char error[64];
        sprintf(error, "Unable to read file %s\n", argv[1]);
        perror(error);
        return 2;
    }

    puts("~~~ Input Sudoku ~~~");
    print_grid(gMOAS->to_solve);
    printf("Empty Squares: %ld\n", gMOAS->n_empty_sq);

    // Solve the puzzle
    omp_set_num_threads(thread_count);
    double start = omp_get_wtime();

    uint_fast8_t *mask =
        (uint_fast8_t *)calloc(gMOAS->n_empty_sq, sizeof(uint_fast8_t));
    #pragma omp parallel firstprivate(mask)
    {
        #pragma omp single
        { solve(0, mask); }
    }
    free(mask);
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

    free(gMOAS->to_solve);
    free(gMOAS->empty_sqs);
    free(gMOAS);
    printf("Total Time %lfs\n", (finish - start));
    return 0;
}
