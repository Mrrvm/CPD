#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdint.h>
#include <inttypes.h>

#define INIT_TAG 1
#define DIE_TAG 2
#define WORK_TAG 3

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

void exit_colony(int ntasks) {
    int id;

    for (id = 1; id < ntasks; ++id) {
        MPI_Send(0, 0, MPI_INT, id, DIE_TAG, MPI_COMM_WORLD);
    }
}

void print_grid() {
    int x, y;

    for (x = 0; x < gMOAS->n; x++) {
        for (y = 0; y < gMOAS->n; y++) {
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

void read_file(const char *filename, int ntasks) {
    FILE *sudoku_file;
    int box_size;
    int num;
    int x, y, id, n;

    sudoku_file = fopen(filename, "re");
    if (sudoku_file == NULL) {
        fprintf(stderr, "Could not open file\n");
        exit_colony(ntasks);
    }

    if (fscanf(sudoku_file, "%d", &box_size) == EOF) {
        fprintf(stderr, "Could not read file\n");
        exit_colony(ntasks);
    }
    for (id = 1; id < ntasks; ++id) {
        MPI_Send(&box_size, 1, MPI_INT, id, INIT_TAG, MPI_COMM_WORLD);
    }

    gMOAS = (moas_t *)malloc(sizeof(moas_t));
    gMOAS->box_size = box_size;
    gMOAS->n = box_size * box_size;
    /* Init our bit mask */
    gMOAS->bits = (int *)calloc(gMOAS->n + 1, sizeof(int));
    for (n = 1; n < gMOAS->n + 1; n++) {
        gMOAS->bits[n] = 1 << n;
    }

    /* Read the file */
    gMOAS->known = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->grid = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->rows = (int *)calloc(gMOAS->n, sizeof(int));
    gMOAS->cols = (int *)calloc(gMOAS->n, sizeof(int));
    gMOAS->squares = (int *)calloc(gMOAS->n, sizeof(int));
    for (x = 0; x < gMOAS->n; x++) {
        gMOAS->known[x] = (int *)calloc(gMOAS->n, sizeof(int));
        gMOAS->grid[x] = (int *)calloc(gMOAS->n, sizeof(int));
        for (y = 0; y < gMOAS->n; y++) {
            fscanf(sudoku_file, "%d", &num);
            //set_cell(x, y, num);
            gMOAS->known[x][y] = num;
            for (id = 1; id < ntasks; ++id) {
                MPI_Send(&num, 1, MPI_INT, id, INIT_TAG, MPI_COMM_WORLD);
            }
        }
    }

    fclose(sudoku_file);
}

void build_map() {

    MPI_Status status;
    int x, y, n, msg;

    gMOAS = (moas_t *)malloc(sizeof(moas_t));
    MPI_Recv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    gMOAS->box_size = msg;
    gMOAS->n = msg * msg;
    /* Init our bit mask */
    gMOAS->bits = (int *)calloc(gMOAS->n + 1, sizeof(int));
    for (n = 1; n < gMOAS->n + 1; n++) {
        gMOAS->bits[n] = 1 << n;
    }

    gMOAS->known = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->grid = (int **)calloc(gMOAS->n, sizeof(int *));
    gMOAS->rows = (int *)calloc(gMOAS->n, sizeof(int));
    gMOAS->cols = (int *)calloc(gMOAS->n, sizeof(int));
    gMOAS->squares = (int *)calloc(gMOAS->n, sizeof(int));
    for (x = 0; x < gMOAS->n; x++) {
        gMOAS->known[x] = (int *)calloc(gMOAS->n, sizeof(int));
        gMOAS->grid[x] = (int *)calloc(gMOAS->n, sizeof(int));
        for (y = 0; y < gMOAS->n; y++) {
            MPI_Recv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            //set_cell(x, y, msg);
            gMOAS->known[x][y] = msg;
        }
    }
}


void master(){

    int ntasks, id;
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

    // Read file and send to slaves
    read_file("testfiles/4x4.txt", ntasks);

    print_grid();

    // Send exit signal
    exit_colony(ntasks);
}

void slave() {

    MPI_Status status;
    int msg;

    build_map();
    print_grid();

    while(1) {
        MPI_Recv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == DIE_TAG) {
            return;
        }
        else if(status.MPI_TAG == WORK_TAG) {
            // Here we work on the task or else we do an interrupt
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
        slave();
    }
    MPI_Finalize();
    return 0;
}
