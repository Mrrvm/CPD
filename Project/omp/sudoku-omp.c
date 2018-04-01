// Sudoku Solver
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

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
    square **empty_sq;
} sudoku;

typedef struct node_struct {
    uint_fast8_t *plays;
    uint_fast8_t curr_play;
    struct node_struct *prev;
} node;

typedef struct queue_struct {
    node *head;
    node *tail;
    uint_fast8_t size;
} queue;

sudoku *to_solve;
queue *q;

node *create_node() {

    node *new_node = (node*) malloc(sizeof(node));

    new_node->curr_play = 0;
    new_node->prev = NULL;
    new_node->plays = (uint8_t *)malloc(to_solve->n_plays * sizeof(uint8_t));
    return new_node;
}

queue *init_queue() {

    queue *new_queue = (queue*) malloc(sizeof(queue));

    new_queue->size = 0;
    new_queue->head = NULL;
    new_queue->tail = NULL;

    return new_queue;
}

node *dequeue() {
    
    node *item;
    if(q->size == 0)
        return NULL;
    item = q->head;
    q->head = (q->head)->prev;
    q->size--;
    return item;
}

void free_queue() {
    
    node *item;
    while(q->size != 0) {
        item = dequeue(q);
        free(item);
    }
    free(q);
}

int enqueue(node *item) {
 
    if (q == NULL || item == NULL) {
        return false;
    }

    if (q->size == 0) {
        q->head = item;
        q->tail = item;

    } else {
        q->tail->prev = item;
        q->tail = item;
    }
    q->size++;
    return true;
}

int set_play(node *to_play, int play) {

    to_play->plays[to_play->curr_play] = play;
    to_play->curr_play++;
    return to_play->curr_play;
}

void print_node(node *to_print) {

    printf("Plays [");
    for (int i = 0; i < to_print->curr_play; ++i) {
        printf("%d -> ", to_print->plays[i]);
    }
    printf("]\n");
}

void print_queue() {

    node *item;
    item = q->head;
    printf("Queue state\n");
    while(item != NULL) {
        print_node(item);
        item = item->prev;
    }  
    printf("\n");
}

void print_error(char *error) {
    fprintf(stderr, error);
    exit(EXIT_FAILURE);
}

void free_sudoku() {
    for (int i = 0; i < to_solve->n; i++) {
        free(to_solve->grid[i]);
    }
    free(to_solve);
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
            if (to_solve->grid[i + row_start][j + col_start] == num) {
                return false;
            }
    return true;
}

bool valid_in_row_and_col(int row, int col, int num) {
    for (int i = 0; i < to_solve->n; i++) {
        if (to_solve->grid[i][col] == num || to_solve->grid[row][i] == num) {
            return false;
        }
    }
    return true;
}

// Check if it is safe to put number in position
bool safe(int row, int col, int num) {
    return valid_in_box(row, col, num) && valid_in_row_and_col(row, col, num);
}

bool safe_by_square(square *to_test, int num) {
    return valid_in_box(to_test->row, to_test->col, num) &&
           valid_in_row_and_col(to_test->row, to_test->col, num);
}

void set_by_ptr(int ptr, uint_fast8_t val) {
    to_solve->grid[to_solve->empty_sq[ptr]->row][to_solve->empty_sq[ptr]->col] = val;
}

square *get_square_by_ptr(int ptr) {
    return to_solve->empty_sq[ptr];
}

void cpy_plays_to_sudoku(node *item) {

    for (int i = 0; i < to_solve->n_plays; ++i) {
        set_by_ptr(i, item->plays[i]);
    }
    return;
}

int solve() {
    
    int finish = 0, i = 0, tid;
    node *new_node, *q_node;
    q = init_queue();

    // Initialize queue
    #pragma omp parallel private(new_node)
    #pragma omp for 
    for (i = 1; i <= to_solve->n; ++i) {
        if (safe_by_square(get_square_by_ptr(0), i)) {
            new_node = create_node();
            set_play(new_node, i);
            #pragma omp critical 
            enqueue(new_node);
        }
    }
    #pragma omp barrier


    if(q->size == 0)
    // No solution
        return 0;

    print_queue();
    // Work on the queue
    while(q->size != 0) {

        // Get one node from queue
        q_node = NULL;
        q_node = dequeue();
        
        if(q_node->curr_play == to_solve->n_plays) {
        // Solution was found
            finish++;
            cpy_plays_to_sudoku(q_node);
        }

        #pragma omp parallel private(new_node, tid)
        {
            tid = omp_get_thread_num();
            if(!finish) {
                #pragma omp for
                for (i = 1; i <= to_solve->n; i++) {
                    printf("[Thread %d]: trying %d on play %d\n", tid, i, q_node->curr_play); fflush(stdout);
                    if (safe_by_square(get_square_by_ptr(q_node->curr_play), i)) {
                        new_node = create_node();
                        memcpy(new_node->plays, q_node->plays, (q_node->curr_play)*sizeof(uint8_t));
                        new_node->curr_play = q_node->curr_play;
                        set_play(new_node, i);
                        
                        #pragma omp critical 
                        {
                            printf("[Thread %d]: enqueued - %d\n", tid, q->size); fflush(stdout);
                            enqueue(new_node);

                        }
                    }          
                }
                #pragma omp single
                free(q_node);
            }
        }
        printf("queue size %d\n", q->size);
    }

    if(finish) {
        return true;
    }
    return false;
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

    if (iter == 0) {
        fclose(sudoku_file);
        return 1;
    }

    // Create aux array of empty squares
    to_solve->n_plays = iter;
    to_solve->empty_sq = (square **)malloc(iter * sizeof(square *));

    for (int i = 0; i < iter; i++) {
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
    if (solve()) {
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
