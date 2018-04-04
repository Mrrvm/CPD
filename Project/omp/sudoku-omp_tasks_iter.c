#include <errno.h>
#include <inttypes.h>
#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_ARGS 2
#define EMPTY 0
#define MIN(X, Y) ((X)<(Y)?(X):(Y))

typedef struct square_struct {
    int_fast8_t row;
    int_fast8_t col;
    int_fast8_t box;
} square;

typedef uint_fast8_t **sudoku;

// Mother of all sudokus
typedef struct moas_t {
    square **empty_sq;
    sudoku to_solve;
    int_fast32_t n;
    int_fast32_t box_size;
    int_fast32_t n_empty_sq;
} moas;

typedef struct task_log_t {
    sudoku state;
    int next_ptr;
    int steps;
} task_log;

int gDONE;
moas *gMOAS;

void print_error(char *error) {
    fprintf(stderr, error);
    exit(EXIT_FAILURE);
}

int read_file(const char *filename) {
    FILE *sudoku_file;
    int box_size;
    uint_fast32_t iter;
    uint8_t num;

    /* Opens file */
    sudoku_file = fopen(filename, "r");
    if (sudoku_file == NULL)
        print_error("Could not open file\n");

    /* Scans first line (aka the square size) */
    if (fscanf(sudoku_file, "%d", &box_size) == EOF)
        print_error("Could not read file\n");

    gMOAS = (moas *)malloc(sizeof(moas));
    if (gMOAS == NULL) {
        print_error("Unable to init MOAS");
    }

    gMOAS->box_size = box_size;
    gMOAS->n = box_size * box_size;

    gMOAS->to_solve = (sudoku)malloc(gMOAS->n * sizeof(uint_fast8_t *));
    for (int i = 0; i < gMOAS->n; i++) {
        gMOAS->to_solve[i] =
            (uint_fast8_t *)malloc(gMOAS->n * sizeof(uint_fast8_t));
    }

    /* Read the file */
    iter = 0;
    for (int i = 0; i < gMOAS->n; i++) {
        for (int j = 0; j < gMOAS->n; j++) {
            fscanf(sudoku_file, "%2" SCNu8, &num);

            gMOAS->to_solve[i][j] = num;
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
    gMOAS->n_empty_sq = iter;
    gMOAS->empty_sq = (square **)malloc(iter * sizeof(square *));

    for (uint_fast32_t i = 0; i < iter; i++) {
        gMOAS->empty_sq[i] = (square *)malloc(sizeof(square));
    }

    iter = 0;
    for (int i = 0; i < gMOAS->n; i++) {
        for (int j = 0; j < gMOAS->n; j++) {
            if (gMOAS->to_solve[i][j] == 0) {
                gMOAS->empty_sq[iter]->row = i;
                gMOAS->empty_sq[iter]->col = j;
                gMOAS->empty_sq[iter]->box =
                    (i / gMOAS->box_size) * gMOAS->box_size + j / gMOAS->box_size;

                iter++;
            }
        }
    }

    fclose(sudoku_file);
    return 0;
}

// Free sudoku typedef
void free_sudoku(sudoku to_free) {
    for (int i = 0; i < gMOAS->n; i++) {
        free(to_free[i]);
    }
    free(to_free);
}

// Free task log typedef
void free_task_log(task_log *to_free) {
    free(to_free->state);

    free(to_free);
}

// Free global MOAS
void free_gMOAS() {
    free_sudoku(gMOAS->to_solve);
    for (int i = 0; i < gMOAS->n_empty_sq; ++i) {
        free(gMOAS->empty_sq[i]);
    }

    free(gMOAS->empty_sq);
    free(gMOAS);
}

// Print grid
void print_grid(sudoku to_print) {
    for (int row = 0; row < gMOAS->n; row++) {
        for (int col = 0; col < gMOAS->n; col++) {
            printf("%2d ", to_print[row][col]);
        }
        printf("\n");
    }
    printf("\n");
}

// Check box, row and column for safety
bool safe(sudoku to_check, square *to_test, int num) {
    uint_fast8_t row_start = to_test->row - to_test->row % gMOAS->box_size;
    uint_fast8_t col_start = to_test->col - to_test->col % gMOAS->box_size;

    for (int i = 0; i < gMOAS->n; i++) {
        if (to_check[i][to_test->col] == num || to_check[to_test->row][i] == num) {
            return false;
        }
    }

    for (int i = 0; i < gMOAS->box_size; i++)
        for (int j = 0; j < gMOAS->box_size; j++)
            if (to_check[i + row_start][j + col_start] == num) {
                return false;
            }

    return true;
}

void copy_to_gMOAS(sudoku solved) {
    for (int i = 0; i < gMOAS->n_empty_sq; i++) {
        gMOAS->to_solve[gMOAS->empty_sq[i]->row][gMOAS->empty_sq[i]->col] =
            solved[gMOAS->empty_sq[i]->row][gMOAS->empty_sq[i]->col];
    }
}

sudoku new_state_copy(sudoku old_state) {
    sudoku new_state = (sudoku)malloc(gMOAS->n * sizeof(uint_fast8_t *));
    for (int i = 0; i < gMOAS->n; i++) {
        new_state[i] = (uint_fast8_t *)malloc(gMOAS->n * sizeof(uint_fast8_t));
        memcpy(new_state[i], old_state[i], gMOAS->n * sizeof(uint_fast8_t));
    }
    return new_state;
}

task_log *new_task_log(sudoku old_state, int next_ptr, int steps) {
    task_log *new = (task_log*) malloc(sizeof(task_log));

    new->state = new_state_copy(old_state);
    new->next_ptr = next_ptr;
    new->steps = steps;
}

void solve(int id, sudoku state) {
    if (gDONE) {
        return;
    }

    /* if in the last solve layer check if solved and start popping */
    if (id == gMOAS->n_empty_sq - 1) {
        for (int i = 1; i <= gMOAS->n; i++) {
            if (safe(state, gMOAS->empty_sq[id], i)) {
                state[gMOAS->empty_sq[id]->row][gMOAS->empty_sq[id]->col] = i;
                copy_to_gMOAS(state);
                gDONE = true;
                return;
            }
        }
        return;
    }

    /* try each son solution */
    #pragma omp parallel
    {
        #pragma omp for
        for (int i = 1; i <= gMOAS->n; i++) {
            #pragma omp task untied
            {
                if (safe(state, gMOAS->empty_sq[id], i)) {
                    state[gMOAS->empty_sq[id]->row][gMOAS->empty_sq[id]->col] = i;
                    sudoku new_state = new_state_copy(state);
                    solve(id + 1, new_state);
                }
            }
        }

        #pragma omp taskwait
    }
    free_sudoku(state);
}

void solve_task_sudoku (task_log *task_l) {
    task_log *new_task_l;
    int nplays, *v_plays;
    int ptr, base_ptr, aim_ptr;

    if (gDONE) {
        free(task_l);

        return;
    }

    base_ptr = task_l->next_ptr;

    nplays = MIN(gMOAS->n_empty_sq - task_l->next_ptr, task_l->steps);
    aim_ptr = base_ptr + nplays;

    if (nplays == 0) {
        return;    
    }

    v_plays = (int *) malloc(nplays * sizeof(int));

    ptr = 0;
    v_plays[0] = 1;

    while (1) {
        /* This square is empty. */

        /* Check if branch options are emptied. */
        if (v_plays[ptr] > gMOAS->n) {
            if (ptr == 0) {
                /* Task done */

                break;
            }
            /* Backtrack */
            ptr--;

            task_l->state[gMOAS->empty_sq[ptr+base_ptr]->row][gMOAS->empty_sq[ptr+base_ptr]->col] = 0;
            continue;
        }

        /* Check if next play is valid. */
        if (safe(task_l->state, gMOAS->empty_sq[ptr+base_ptr], v_plays[ptr])) {
        
            task_l->state[gMOAS->empty_sq[ptr+base_ptr]->row][gMOAS->empty_sq[ptr+base_ptr]->col] = v_plays[ptr];
            v_plays[ptr]++; /* always to next */

            ptr++;
            if (ptr < nplays) {
                /* Branch */

                v_plays[ptr] = 1;
            } else if (aim_ptr < gMOAS->n_empty_sq) {
                /* Reached aim level */

                /* Create new task */
                new_task_l = (task_log*) new_task_log ( task_l->state,
                                              aim_ptr, 2*task_l->steps );
                
                /* Launch */
                #pragma omp task untied
                {
                    solve_task_sudoku(new_task_l);
                }

                new_task_l = NULL;
            } else {
                /* Has solved all :) */

                gDONE = 1;
                copy_to_gMOAS(task_l->state);

                free(task_l);                
                return;
            }
        } else {
            /* Try again */

            v_plays[ptr]++;
        }
    }

    #pragma omp taskwait

    free(task_l);
}

int main(int argc, char const *argv[]) {
    clock_t begin, end;
    task_log *orig_task_l;

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

    print_grid(gMOAS->to_solve);

    begin = clock();
    // Solve the puzzle
    orig_task_l = new_task_log(new_state_copy(gMOAS->to_solve), 0, 2);
    solve_task_sudoku(orig_task_l);
    end = clock();

    if (gDONE) {
        print_grid(gMOAS->to_solve);
        printf("Solved Sudoku\n");
        // Solution found
    } else {
        printf("Did not solve Sudoku\n");
        // No solution
    };

    free_gMOAS();
    printf("Total Time %f\n", (double)(end - begin) / CLOCKS_PER_SEC);
    return 0;
}
