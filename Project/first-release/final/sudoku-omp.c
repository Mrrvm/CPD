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

#define FRAC_THRES 0.9
#define EXTRA_TASKS 10

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
} task_log;

int gDONE;
int idle;
moas *gMOAS;
int treshold;

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

task_log *new_task_log(sudoku old_state, int next_ptr) {
    task_log *new = (task_log *)malloc(sizeof(task_log));

    new->state = new_state_copy(old_state);
    new->next_ptr = next_ptr;

    return new;
}

void solve_task_sudoku(task_log *task_l) {
    task_log *task_top, *new_task_l;
    int nplays, *v_plays;
    int ptr, base_ptr;
    int top;

    if (gDONE) {
        free(task_l);

        return;
    }

    base_ptr = task_l->next_ptr;

    nplays = gMOAS->n_empty_sq - base_ptr;

    v_plays = (int *)malloc(nplays * sizeof(int));

    task_top = (task_log *)new_task_log(task_l->state, base_ptr);

    top = ptr = 0;
    v_plays[0] = 1;

    while (1) {
        /* This square is empty. */

        if (gDONE) {
            free(task_l);
            free(task_top);

            return;
        }

        /* Check if branch options are emptied. */
        if (v_plays[ptr] > gMOAS->n) {
            if (ptr == top) {
                /* Task done */

                break;
            }

            /* Backtrack */
            ptr--;
            task_l->state[gMOAS->empty_sq[ptr + base_ptr]->row]
            [gMOAS->empty_sq[ptr + base_ptr]->col] = 0;

            continue;
        }

        /* Fork task workload from top if idle threads are detected. */
        if ((base_ptr - top) < treshold && top < ptr && top < (nplays - 1) &&
                idle > 0) {

            if (safe(task_top->state, gMOAS->empty_sq[top + base_ptr],
                     v_plays[top])) {

                if (v_plays[top] > gMOAS->n) {
                    /* Branch top down in the branch currenlty being explored in this
                     * task. */

                    task_top->state[gMOAS->empty_sq[top + base_ptr]->row]
                    [gMOAS->empty_sq[top + base_ptr]->col] =
                        task_l->state[gMOAS->empty_sq[top + base_ptr]->row]
                        [gMOAS->empty_sq[top + base_ptr]->col];

                    top++;
                } else {

                    #pragma omp atomic
                    idle--;

                    // printf("Launch sub-task, level %d, option %d\n", top,
                    // v_plays[top]);

                    /* Create new task */
                    new_task_l =
                        (task_log *)new_task_log(task_top->state, (base_ptr + top + 1));

                    new_task_l->state[gMOAS->empty_sq[top + base_ptr]->row]
                    [gMOAS->empty_sq[top + base_ptr]->col] =
                        v_plays[top];

                    #pragma omp task firstprivate(new_task_l)
                    {
                        solve_task_sudoku(new_task_l);
                    }

                    new_task_l = NULL;

                    v_plays[top]++; /* always to next */

                    if (v_plays[top] > gMOAS->n) {
                        /* Branch top down in the branch currenlty being explored in this
                         * task. */

                        task_top->state[gMOAS->empty_sq[top + base_ptr]->row]
                        [gMOAS->empty_sq[top + base_ptr]->col] =
                            task_l->state[gMOAS->empty_sq[top + base_ptr]->row]
                            [gMOAS->empty_sq[top + base_ptr]->col];

                        top++;
                    }
                }
            } else {
                v_plays[top]++;
            }
        }

        /* Check if next play is valid. */
        if (safe(task_l->state, gMOAS->empty_sq[ptr + base_ptr], v_plays[ptr])) {
            // print_branch(ptr+base_ptr, v_plays[ptr]);

            task_l->state[gMOAS->empty_sq[ptr + base_ptr]->row]
            [gMOAS->empty_sq[ptr + base_ptr]->col] = v_plays[ptr];
            v_plays[ptr]++; /* always to next */

            if (v_plays[ptr] > gMOAS->n && ptr == top) {
                task_top->state[gMOAS->empty_sq[ptr + base_ptr]->row]
                [gMOAS->empty_sq[ptr + base_ptr]->col] = v_plays[ptr];

                top++;
            }

            ptr++;
            if (ptr < nplays) {
                /* Branch down */

                v_plays[ptr] = 1;
            } else {
                /* Has solved all :) */

                #pragma omp critical
                {
                    if (gDONE == 0) {
                        gDONE = 1;
                        copy_to_gMOAS(task_l->state);
                    }
                }

                free(task_l);
                return;
            }
        } else {
            /* Branch sideways */
            v_plays[ptr]++;
        }
    }

    // printf("*** Task: b:%d s:%d a:%d DONE ***\n", base_ptr, nplays, aim_ptr);

    // printf("Level: %d, Branchs:%d\n", task_l->level, cnt);
    free(task_l);
    free(task_top);
    free(v_plays);

    // printf("*** Task: b:%d DONE ***\n", base_ptr);

    #pragma omp atomic
    idle++;

    return;
}

int main(int argc, char const *argv[]) {
    task_log *orig_task_l;
    int thread_count = 4;

    if (argc < N_ARGS) {
        char error[64];
        sprintf(error, "Usage: %s filename\n", argv[0]);
        print_error(error);
    } else if (argc == 3) {
        thread_count = atoi(argv[2]);
    }

    if (read_file(argv[1]) != 0) {
        char error[64];
        sprintf(error, "Unable to read file %s\n", argv[1]);
        print_error(error);
    }

    treshold = (int)(FRAC_THRES * (double)gMOAS->n_empty_sq);
    idle = EXTRA_TASKS;
    omp_set_num_threads(thread_count);

    puts("~~~ Input Sudoku ~~~");
    print_grid(gMOAS->to_solve);

    // Solve the puzzle
    orig_task_l = new_task_log(new_state_copy(gMOAS->to_solve), 0);
    double start = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp atomic
        idle++;

        #pragma omp single
        {
            idle--;
            solve_task_sudoku(orig_task_l);
        }

        #pragma omp taskwait
    }

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

    free_gMOAS();
    printf("%lf\n", (finish - start));
    return 0;
}
