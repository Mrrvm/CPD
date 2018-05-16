#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>

typedef struct mask {
    __uint128_t *rows;
    __uint128_t *cols;
    __uint128_t *squares;
    __uint128_t *bits;
    int **grid;
} mask_t;

typedef struct moas {
    int n;
    int box_size;
    int **known;
    mask_t *mask;
} moas_t;

moas_t *gMOAS;

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

void free_gMOAS() {
    for (int i = 0; i < gMOAS->n; i++) {
        free(gMOAS->->known[i]);
        free((gMOAS->mask)->grid[i]);
    }
    free(gMOAS->known);
    free(gMOAS->grid);
    free((gMOAS->mask)->rows);
    free((gMOAS->mask)->cols);
    free((gMOAS->mask)->squares);
    free((gMOAS->mask)->bits);
    free(gMOAS);
}

void print_grid(mask_t *mask) {
    for (int x = 0; x < gMOAS->n; x++) {
        for (int y = 0; y < gMOAS->n; y++) {
            if (gMOAS->known[x][y]) {
                printf("%2d ", gMOAS->known[x][y]);
            } else {
                printf("%2d ", mask->grid[x][y]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

void exit_colony(int ntasks) {
    int id;
    for (id = 1; id < ntasks; ++id) {
        MPI_Send(0, 0, MPI_INT, id, DIE_TAG, MPI_COMM_WORLD);
    }
}

void solve(int total) {
    int pos = 0;
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


void read_file(const char *filename, int ntasks) {
    FILE *sudoku_file;
    int box_size, num;

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
    gMOAS->known = (int **)calloc(gMOAS->n, sizeof(int *));
    (gMOAS->mask)->grid = (int **)calloc(gMOAS->n, sizeof(int *));
    (gMOAS->mask)->rows = (int *)calloc(gMOAS->n+1, sizeof(int));
    (gMOAS->mask)->cols = (int *)calloc(gMOAS->n+1, sizeof(int));
    (gMOAS->mask)->squares = (int *)calloc(gMOAS->n+1, sizeof(int));
    (gMOAS->mask)->bits = (int *)calloc(gMOAS->n + 1, sizeof(int));
    for (int i = 1; i < gMOAS->n + 1; i++) {
        gMOAS->bits[i] = 1 << n;
    }
    for (int i = 0; i < gMOAS->n; i++) {
        gMOAS->known[i] = (int *)calloc(gMOAS->n, sizeof(int));
        gMOAS->grid[i] = (int *)calloc(gMOAS->n, sizeof(int));
        for (int j = 0; j < gMOAS->n; j++) {
            fscanf(sudoku_file, "%d", &num);
            set_cell(i, j, num);
            gMOAS->known[i][j] = num;
            MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }
    }
    fclose(sudoku_file);
}

void build_map() {

    int box_size, num;

    MPI_Bcast(&box_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    gMOAS = (moas_t *)malloc(sizeof(moas_t));
    gMOAS->box_size = box_size;
    gMOAS->n = box_size * box_size;
    gMOAS->known = (int **)calloc(gMOAS->n, sizeof(int *));
    (gMOAS->mask)->grid = (int **)calloc(gMOAS->n, sizeof(int *));
    (gMOAS->mask)->rows = (int *)calloc(gMOAS->n+1, sizeof(int));
    (gMOAS->mask)->cols = (int *)calloc(gMOAS->n+1, sizeof(int));
    (gMOAS->mask)->squares = (int *)calloc(gMOAS->n+1, sizeof(int));
    (gMOAS->mask)->bits = (int *)calloc(gMOAS->n + 1, sizeof(int));
    for (int i = 1; i < gMOAS->n + 1; i++) {
        gMOAS->bits[i] = 1 << n;
    }
    for (int i = 0; i < gMOAS->n; i++) {
        gMOAS->known[i] = (int *)calloc(gMOAS->n, sizeof(int));
        gMOAS->grid[i] = (int *)calloc(gMOAS->n, sizeof(int));
        for (int j = 0; j < gMOAS->n; j++) {
            MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
            set_cell(i, j, num);
            gMOAS->known[i][j] = num;
        }
    }
}

void master(){

    int ntasks;
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

    // Read file and send to slaves
    read_file("testfiles/4x4.txt", ntasks);


    print_grid();
    // Send exit signal
    exit_colony(ntasks);
    free_gMOAS();
}

void slave(int my_id) {

    MPI_Status status;
    int msg;

    build_map();
    print_grid();

    while(1) {
        MPI_Recv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == DIE_TAG) {
            free_gMOAS();
            return;
        }
    }
}

int main (int argc, char *argv[]) {

    int my_id;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
    if (my_id == 0) {
        master();
    } else {
        slave(my_id);
    }
    MPI_Finalize();
    return 0;
}
