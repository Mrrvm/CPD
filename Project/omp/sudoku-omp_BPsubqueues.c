// Sudoku Solver
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
#define MAX_LEVELS 30

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
    square **empty_sq;
} sudoku;

typedef struct node_struct {
    sudoku *plays;
    struct node_struct *prev;
    int next_ptr;
} node;

typedef struct queue_struct {
    node *head;
    node *tail;
    int size;
} queue;

sudoku *to_solve;
queue *q;

queue *init_queue() {

    queue *new_queue = (queue *)malloc(sizeof(queue));

    new_queue->size = 0;
    new_queue->head = NULL;
    new_queue->tail = NULL;

    return new_queue;
}

node *dequeue(queue *to_dequeue) {

    node *item;
    if (to_dequeue->size == 0)
        return NULL;
    item = to_dequeue->head;
    to_dequeue->head = (to_dequeue->head)->prev;
    to_dequeue->size--;
    return item;
}

int enqueue(node *item, queue *to_enqueue) {

    if (to_enqueue == NULL || item == NULL) {
        return false;
    }

    if (to_enqueue->size == 0) {
        to_enqueue->head = item;
        to_enqueue->tail = item;
    } else {
        to_enqueue->tail->prev = item;
        to_enqueue->tail = item;
    }
    to_enqueue->size++;
    return true;
}

void print_error(char *error) {
    fprintf(stderr, error);
    exit(EXIT_FAILURE);
}

void free_sudoku(sudoku *plays) {
    for (int i = 0; i < to_solve->n; i++) {
        free(plays->grid[i]);
    }
    free(plays);
}

void free_node(node *item) {
    free_sudoku(item->plays);
    free(item);
}

void free_queue(queue *to_free) {

    node *item;
    while (q->size != 0) {
        item = dequeue(to_free);
        free_node(item);
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
void print_grid(sudoku *plays) {
    for (int row = 0; row < to_solve->n; row++) {
        for (int col = 0; col < to_solve->n; col++)
            printf("%2d ", plays->grid[row][col]);
        printf("\n");
    }
    printf("\n");
}

void print_queue(queue *to_print) {

    node *item;
    item = to_print->head;
    printf("Queue state\n");
    while (item != NULL) {
        print_grid(item->plays);
        item = item->prev;
        printf("\n");
    }
}

// Check box
bool valid_in_box(sudoku *plays, int row, int col, int num) {
    uint8_t row_start = row - row % to_solve->box_size;
    uint8_t col_start = col - col % to_solve->box_size;

    for (int i = 0; i < to_solve->box_size; i++)
        for (int j = 0; j < to_solve->box_size; j++)
            if (plays->grid[i + row_start][j + col_start] == num) {
                return false;
            }
    return true;
}

bool valid_in_row_and_col(sudoku *plays, int row, int col, int num) {
    for (int i = 0; i < to_solve->n; i++) {
        if (plays->grid[i][col] == num || plays->grid[row][i] == num) {
            return false;
        }
    }
    return true;
}

// Check if it is safe to put number in position
bool safe(sudoku *plays, int row, int col, int num) {
    return valid_in_box(plays, row, col, num) &&
           valid_in_row_and_col(plays, row, col, num);
}

bool safe_by_square(sudoku *plays, square *to_test, int num) {
    return valid_in_box(plays, to_test->row, to_test->col, num) &&
           valid_in_row_and_col(plays, to_test->row, to_test->col, num);
}

void set_by_ptr(sudoku *plays, int ptr, uint_fast8_t val) {
    plays->grid[to_solve->empty_sq[ptr]->row][to_solve->empty_sq[ptr]->col] = val;
}

square *get_square_by_ptr(int ptr) {
    return to_solve->empty_sq[ptr];
}

node *create_node(sudoku *to_copy) {

    node *new_node = (node *)malloc(sizeof(node));

    new_node->prev = NULL;
    new_node->plays = init_sudoku(to_solve->box_size);

    for (int i = 0; i < to_solve->n; i++) {
        for (int j = 0; j < to_solve->n; j++) {
            (new_node->plays)->grid[i][j] = to_copy->grid[i][j];
        }
    }

    return new_node;
}

void merge_queues(queue *src, queue *dest) {

    (dest->tail)->prev = src->head;
    dest->tail = src->tail;
    dest->size += src->size;
    return;
}

void cpy_final_plays(sudoku *plays) {

    for (int i = 0; i < to_solve->n_plays; ++i) {
        to_solve->grid[to_solve->empty_sq[i]->row][to_solve->empty_sq[i]->col] =
            plays->grid[to_solve->empty_sq[i]->row][to_solve->empty_sq[i]->col];
    }
    return;
}

int solve() {

    int finish = 0;
    q = init_queue();

    // Initialize queue
    #pragma omp parallel
    {
        #pragma omp for nowait
        for (int i = 1; i <= to_solve->n; i++) {
            if (safe_by_square(to_solve, get_square_by_ptr(0), i)) {
                node *new_node;
                new_node = create_node(to_solve);
                new_node->next_ptr = 1;
                set_by_ptr(new_node->plays, 0, i);
                #pragma omp critical
                enqueue(new_node, q);
            }
        }
    }

    if (q->size == 0)
        // No solution
        return 0;

    for (int j = 0; j <= q->size; ++j) {
        #pragma omp task shared(finish)
        if (!finish) {
            node *q_node = NULL;
            // Get one node from queue
            #pragma omp critical
            {
                q_node = dequeue(q);
            }

            // print_grid(q_node->plays);

            if (q_node != NULL) {

                int n_levels = (to_solve->n_plays - q_node->next_ptr > MAX_LEVELS)
                               ? MAX_LEVELS
                               : to_solve->n_plays - q_node->next_ptr;
                queue *priv_q = init_queue();
                int ptr = 0;
                uint8_t *plays = (uint8_t *)malloc(n_levels * sizeof(uint8_t));
                plays[0] = 1;

                while (1) {

                    if (!finish) {
                        if (plays[ptr] > to_solve->n) {
                            if (ptr == 0) {
                                if (priv_q->size > 0) {
                                    #pragma omp critical
                                    merge_queues(priv_q, q);
                                }
                                free(priv_q);
                                break;
                            } else {
                                // Backtrack
                                plays[ptr] = 1;
                                ptr--;
                                set_by_ptr(q_node->plays, q_node->next_ptr, 0);
                                q_node->next_ptr--;
                                set_by_ptr(q_node->plays, q_node->next_ptr, 0);
                                continue;
                            }
                        }

                        if (safe_by_square(q_node->plays,
                                           get_square_by_ptr(q_node->next_ptr),
                                           plays[ptr])) {
                            set_by_ptr(q_node->plays, q_node->next_ptr, plays[ptr]);

                            plays[ptr]++;

                            if (ptr == n_levels - 1) {
                                if (q_node->next_ptr == to_solve->n_plays - 1) {
                                    #pragma omp atomic
                                    finish++;
                                    cpy_final_plays(q_node->plays);
                                } else {
                                    node *new_node = create_node(q_node->plays);
                                    new_node->next_ptr = q_node->next_ptr + 1;
                                    enqueue(new_node, priv_q);
                                }
                            } else {
                                q_node->next_ptr++;
                                ptr++;
                                plays[ptr] = 1;
                            }

                        } else {
                            plays[ptr]++;
                        }

                    } else {
                        break;
                    }
                }
                free_node(q_node);
            }
        }
    }

    #pragma omp taskwait
    free_queue(q);
    if (finish)
        return true;
    return false;
}

int read_file(const char *filename) {
    FILE *sudoku_file;
    int box_size;
    uint_fast32_t iter;
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

    if (iter == 0) {
        fclose(sudoku_file);
        return 1;
    }

    // Create aux array of empty squares
    to_solve->n_plays = iter;
    to_solve->empty_sq = (square **)malloc(iter * sizeof(square *));

    for (uint_fast32_t i = 0; i < iter; i++) {
        to_solve->empty_sq[i] = (square *)malloc(iter * sizeof(square));
    }

    iter = 0;
    for (int i = 0; i < to_solve->n; i++) {
        for (int j = 0; j < to_solve->n; j++) {
            if (to_solve->grid[i][j] == 0) {
                to_solve->empty_sq[iter]->row = i;
                to_solve->empty_sq[iter]->col = j;
                to_solve->empty_sq[iter]->box =
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
    int thread_count = 4;
    double begin, end;

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
    omp_set_num_threads(thread_count);

    print_grid(to_solve);
    printf("\n");

    begin = omp_get_wtime();
    // Solve the puzzle
    if (solve()) {
        end = omp_get_wtime();
        print_grid(to_solve);
        printf("Solved Sudoku\n");
        // Solution found
    } else {
        end = omp_get_wtime();
        printf("Did not solve Sudoku\n");
        // No solution
    };

    free_sudoku(to_solve);
    printf("%lf\n", (end - begin));
    return 0;
}
