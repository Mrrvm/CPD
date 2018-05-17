#include <inttypes.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define INIT_TAG 1
#define DIE_TAG 2
#define WORK_TAG 3
#define NO_WORK_TAG 4
#define SOLUTION_TAG 6

#define IDLE 100
#define WORKING 101

typedef unsigned __int128 uint128_t;

typedef struct info {
    int x;
    int y;
    int v;
} info_t;

typedef struct mask {
    __uint128_t *rows;
    __uint128_t *cols;
    __uint128_t *squares;
    __uint128_t *bits;
    info_t *history;
    int history_len;
    int **grid;
} mask_t;

typedef struct moas {
    int n;
    int box_size;
    int n_empty;
    int **known;
    mask_t *mask;
} moas_t;

typedef struct work_type {
    info_t *history;
    int history_len;
} work_t;

moas_t *gMOAS;
int gDONE;

int square(int i, int j) {
    return (i / gMOAS->box_size) * gMOAS->box_size + j / gMOAS->box_size;
}

void set_cell(int i, int j, int n, mask_t *mask) {
    mask->grid[i][j] = n;
    mask->rows[i] |= mask->bits[n];
    mask->cols[j] |= mask->bits[n];
    mask->squares[square(i, j)] |= mask->bits[n];
}

int clear_cell(int i, int j, mask_t *mask) {
    int n = mask->grid[i][j];
    mask->grid[i][j] = 0;
    mask->rows[i] &= ~mask->bits[n];
    mask->cols[j] &= ~mask->bits[n];
    mask->squares[square(i, j)] &= ~mask->bits[n];
    return n;
}

bool is_available(int i, int j, int n, mask_t *mask) {
    return (mask->rows[i] & mask->bits[n]) == 0 &&
           (mask->cols[j] & mask->bits[n]) == 0 &&
           (mask->squares[square(i, j)] & mask->bits[n]) == 0;
}

void exit_colony(int ntasks) {
    int id;
    for (id = 1; id < ntasks; ++id) {
        MPI_Send(0, 0, MPI_INT, id, DIE_TAG, MPI_COMM_WORLD);
    }
}

void add_to_history(int i, int j, int value, mask_t *mask) {
    mask->history[mask->history_len].x = i;
    mask->history[mask->history_len].y = j;
    mask->history[mask->history_len].v = value;
    mask->history_len++;
}

void remove_last_from_history(mask_t *mask) {
    mask->history_len = (mask->history_len == 0) ? 0 : mask->history_len - 1;
}

void restore_from_history(mask_t *mask, info_t *history, int history_len) {
    for (int i = 0; i < history_len; i++) {
        clear_cell(history[i].x, history[i].y, gMOAS->mask);
        set_cell(history[i].x, history[i].y, history[i].v, gMOAS->mask);
        add_to_history(history[i].x, history[i].y, history[i].v, gMOAS->mask);
    }
}

void rewrite_history(int index, int value, mask_t *mask) {
    if (index > mask->history_len) {
        printf("Invalid index to rewrite");
    }
    mask->history[index].v = value;
}

void clear_history(mask_t *mask) {
    mask->history_len = 0;
}

void print_grid(mask_t *mask) {
    int i, j;
    for (i = 0; i < gMOAS->n; i++) {
        for (j = 0; j < gMOAS->n; j++) {
            if (gMOAS->known[i][j]) {
                printf("%2d ", gMOAS->known[i][j]);
            } else {
                printf("%2d ", mask->grid[i][j]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

void free_gMOAS() {
    int i;
    for (i = 0; i < gMOAS->n; i++) {
        free(gMOAS->known[i]);
        free((gMOAS->mask)->grid[i]);
    }
    free(gMOAS->known);
    free(gMOAS->mask->grid);
    free(gMOAS->mask->rows);
    free(gMOAS->mask->cols);
    free(gMOAS->mask->squares);
    free(gMOAS->mask->bits);
    free(gMOAS->mask->history);
    free(gMOAS);
}
bool advance_cell(int i, int j) {
    int n = clear_cell(i, j, gMOAS->mask);
    while (++n <= gMOAS->n) {
        if (is_available(i, j, n, gMOAS->mask)) {
            set_cell(i, j, n, gMOAS->mask);
            add_to_history(i, j, n, gMOAS->mask);
            return true;
        }
    }
    return false;
}

work_t *initial_work(int ntasks, int *top, int *size) {

    int total = 0, acc = 1;
    work_t *work;
    int pos = 0;

    while (acc < (ntasks + INIT_BUFF) && total < gMOAS->n) {
        if(!gMOAS->known[total / gMOAS->n][total % gMOAS->n]) {
            acc *= (gMOAS->n - total);
        }
        total++;
    }

    *size = acc * 2;
    work = (int *)calloc(*size, sizeof(work_t));
    *top = 0;

    /* Explore to depth total */
    while (1) {
        while (pos < total && gMOAS->known[pos / gMOAS->n][pos % gMOAS->n]) {
            ++pos;
        }
        if (pos >= total) {
            /* save this history */
            work[*top].history = work[*top].history_len = (*top)++;

            /* backtrack */
        }

        if (advance_cell(pos / gMOAS->n, pos % gMOAS->n)) {
            ++pos;
        } else {
            if (gMOAS->mask->history_len == 0) {
                break;
            }
            pos = gMOAS->mask->history[gMOAS->mask->history_len - 1].x * gMOAS->n +
                  gMOAS->mask->history[gMOAS->mask->history_len - 1].y;
            remove_last_from_history(gMOAS->mask);
        }
    }

    return work;
}

void solve(int pos, int total) {
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
            if (gMOAS->mask->history_len == 0) {
                break;
            }
            pos = gMOAS->mask->history[gMOAS->mask->history_len - 1].x * gMOAS->n +
                  gMOAS->mask->history[gMOAS->mask->history_len - 1].y;
            remove_last_from_history(gMOAS->mask);
        }
    }
}

void build_map() {
    int box_size, num, i, j, n_empty;

    MPI_Bcast(&box_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    gMOAS = (moas_t *)malloc(sizeof(moas_t));
    gMOAS->box_size = box_size;
    gMOAS->n = box_size * box_size;
    gMOAS->known = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->mask = (mask_t *)malloc(sizeof(mask_t));
    gMOAS->mask->grid = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->mask->rows = calloc(gMOAS->n + 1, sizeof(uint128_t));
    gMOAS->mask->cols = calloc(gMOAS->n + 1, sizeof(uint128_t));
    gMOAS->mask->squares = calloc(gMOAS->n + 1, sizeof(uint128_t));
    gMOAS->mask->bits = calloc(gMOAS->n + 1, sizeof(uint128_t));
    for (i = 1; i < gMOAS->n + 1; i++) {
        gMOAS->mask->bits[i] = 1 << i;
    }
    for (i = 0; i < gMOAS->n; i++) {
        gMOAS->known[i] = (int *)calloc(gMOAS->n, sizeof(int));
        gMOAS->mask->grid[i] = (int *)calloc(gMOAS->n, sizeof(int));
        for (j = 0; j < gMOAS->n; j++) {
            MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
            set_cell(i, j, num, gMOAS->mask);
            gMOAS->known[i][j] = num;
        }
    }

    MPI_Bcast(&gMOAS->n_empty, 1, MPI_INT, 0, MPI_COMM_WORLD);
    gMOAS->mask->history = calloc(gMOAS->n_empty, sizeof(info_t));
}


void read_file(const char *filename, int ntasks) {
    FILE *sudoku_file;
    int box_size, num, i, j;

    sudoku_file = fopen(filename, "re");
    if (sudoku_file == NULL) {
        fprintf(stderr, "Could not open file\n");
        exit_colony(ntasks);
    }

    if (fscanf(sudoku_file, "%d", &box_size) == EOF) {
        fprintf(stderr, "Could not read file\n");
        exit_colony(ntasks);
    }
    MPI_Bcast(&box_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    gMOAS = (moas_t *)malloc(sizeof(moas_t));
    gMOAS->box_size = box_size;
    gMOAS->n = box_size * box_size;
    gMOAS->n_empty = gMOAS->n * gMOAS->n;
    gMOAS->known = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->mask = (mask_t *)malloc(sizeof(mask_t));
    gMOAS->mask->grid = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->mask->rows = calloc(gMOAS->n + 1, sizeof(uint128_t));
    gMOAS->mask->cols = calloc(gMOAS->n + 1, sizeof(uint128_t));
    gMOAS->mask->squares = calloc(gMOAS->n + 1, sizeof(uint128_t));
    gMOAS->mask->bits = calloc(gMOAS->n + 1, sizeof(uint128_t));
    for (i = 1; i < gMOAS->n + 1; i++) {
        gMOAS->mask->bits[i] = 1 << i;
    }
    for (i = 0; i < gMOAS->n; i++) {
        gMOAS->known[i] = (int *)calloc(gMOAS->n, sizeof(int));
        gMOAS->mask->grid[i] = (int *)calloc(gMOAS->n, sizeof(int));
        for (j = 0; j < gMOAS->n; j++) {
            fscanf(sudoku_file, "%d", &num);
            set_cell(i, j, num, gMOAS->mask);
            gMOAS->known[i][j] = num;
            if (num != 0) {
                gMOAS->n_empty--;
            }
            MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Bcast(&gMOAS->n_empty, 1, MPI_INT, 0, MPI_COMM_WORLD);
    gMOAS->mask->history = calloc(gMOAS->n_empty, sizeof(info_t));
    fclose(sudoku_file);
}


void slave(int my_id) {
    MPI_Status status;
    int msg;

    build_map();
    print_grid(gMOAS->mask);
    printf("Available: %d\n", gMOAS->n_empty);
    int total = gMOAS->n * gMOAS->n;
    solve(0, total);

    if (gDONE == true) {
        print_grid(gMOAS->mask);
        printf("Solved Sudoku\n");
        /* for (int i = 0; i < gMOAS->n_empty; i++) { */
        /*     printf("%d,%d %d\n", gMOAS->mask->history[i].x,
         * gMOAS->mask->history[i].y, */
        /*            gMOAS->mask->history[i].v); */
        /* } */
        // Solution found
    } else {
        printf("Did not solve Sudoku\n");
        // No solution
    };

    while (1) {
        MPI_Recv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == DIE_TAG) {
            free_gMOAS();
            return;
        }
    }
}

void master(char *file) {
    int ntasks;
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

    // Read file and send to slaves
    read_file(file, ntasks);

    print_grid(gMOAS->mask);
    // Send exit signal
    exit_colony(ntasks);
    free_gMOAS();
}

int main(int argc, char *argv[]) {
    int my_id;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
    if (my_id == 0) {
        master(argv[1]);
    } else {
        slave(my_id);
    }
    MPI_Finalize();
    return 0;
}

