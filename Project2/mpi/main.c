#include <inttypes.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//#define REDIST_ON 

#define INIT_BUFF 6
#define WORK_TRESH 4
#define STEP_SIZE 10
#define DEPTH_TRESH 6

enum tags {DIE_TAG = 1, RED_TAG, WORK_TAG, NO_WORK_TAG, SOLUTION_TAG};
enum slave_st {IDLE = 100, WORKING, REQUEST};

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

void set_cell(int i, int j, int n) {
    mask_t *mask = gMOAS->mask;
    mask->grid[i][j] = n;
    mask->rows[i] |= mask->bits[n];
    mask->cols[j] |= mask->bits[n];
    mask->squares[square(i, j)] |= mask->bits[n];
}

int clear_cell(int i, int j) {
    mask_t *mask = gMOAS->mask;
    int n = mask->grid[i][j];
    mask->grid[i][j] = 0;
    mask->rows[i] &= ~mask->bits[n];
    mask->cols[j] &= ~mask->bits[n];
    mask->squares[square(i, j)] &= ~mask->bits[n];
    return n;
}

bool is_available(int i, int j, int n) {
    mask_t *mask = gMOAS->mask;
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

void add_to_history(int i, int j, int value) {
    mask_t *mask = gMOAS->mask;
    mask->history[mask->history_len].x = i;
    mask->history[mask->history_len].y = j;
    mask->history[mask->history_len].v = value;
    mask->history_len++;
}

void remove_last_from_history() {
    mask_t *mask = gMOAS->mask;
    mask->history_len = (mask->history_len == 0) ? 0 : mask->history_len - 1;
}

void clear_history() {
    mask_t *mask = gMOAS->mask;
    mask->history_len = 0;
}

void restore_from_history(info_t *history, int history_len) {
    mask_t *mask = gMOAS->mask;
    int i;
    for (i = 0; i < history_len; i++) {
        clear_cell(history[i].x, history[i].y);
        set_cell(history[i].x, history[i].y, history[i].v);
        add_to_history(history[i].x, history[i].y, history[i].v);
    }
}

void copy_history(work_t *work) {
    mask_t *mask = gMOAS->mask;
    int i = 0, len = mask->history_len;
    work->history = calloc(len, sizeof(info_t));
    for(i=0; i < len; i++) {
        work->history[i] = mask->history[i];
    }
    work->history_len = len;
}

void rewrite_history(int index) {
    mask_t *mask = gMOAS->mask;
    if (index > mask->history_len) {
        printf("Invalid index to rewrite");
    }
    if (mask->history[index].x+mask->history[index].y >= gMOAS->n) {
        printf("No fork space in history rewrite");
    }

    (mask->history[index].v)++;
}

int root_history(int root) {
    int i;
    mask_t *mask = gMOAS->mask;
    /* gets first history whose value is not the last possible */
    for (i = root; i < mask->history_len; i++) {

        if (mask->history[mask->history_len].v != gMOAS->n)
            return i;
    }

    return -1;
}

bool advance_cell(int i, int j) {
    mask_t *mask = gMOAS->mask;
    int n = clear_cell(i, j);
    while (++n <= gMOAS->n) {
        if (is_available(i, j, n)) {
            set_cell(i, j, n);
            add_to_history(i, j, n);
            return true;
        }
    }
    return false;
}

void print_grid() {
    int i, j;
    mask_t *mask = gMOAS->mask;
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

int solve_nsteps(int root, int *pos, int total, int nsteps) {
    int i;
    mask_t *mask = gMOAS->mask;
    for (i = 0; i < nsteps; i++) {
        while (*pos < total && gMOAS->known[*pos / gMOAS->n][*pos % gMOAS->n]) {
            ++(*pos);
        }
        if (*pos >= total) {
            gDONE = true;
            return 0;
        }

        if (advance_cell(*pos / gMOAS->n, *pos % gMOAS->n)) {
            ++(*pos);
        } else {
            if (mask->history_len == root) {
                return 0;
            }
            (*pos) = mask->history[mask->history_len - 1].x * gMOAS->n +
                  mask->history[mask->history_len - 1].y;
            remove_last_from_history();
        }
    }

    return 1;
}



void print_history(info_t *history, int length) {
    int i = 0;
    printf("History length %d\n", length);
    for(i=0; i<length; i++) {
        printf("map[%d][%d] = %d\n", history[i].x, history[i].y, history[i].v);
    }
}

void print_work_stack(int ntasks, work_t *stack) {
    int i;
    for(i=0; i<ntasks+INIT_BUFF; i++) {
        printf("Element %d\n", i);
        print_history(stack[i].history, stack[i].history_len);
        printf("\n\n");
    }
}

work_t *initial_work(int ntasks, int *top, int *size) {

    int total = 1, acc = gMOAS->n;
    work_t *stack;
    int pos = 0;
    mask_t *mask = gMOAS->mask;

    /* Calculate number of starter possibilities according to ntasks */
    while (acc < (ntasks + INIT_BUFF) && total < gMOAS->n) {
        if(!gMOAS->known[total / gMOAS->n][total % gMOAS->n]) {
            acc *= (gMOAS->n - total);
        }
        total++;
    }

    while(1) {
        if(gMOAS->known[total / gMOAS->n][total % gMOAS->n]) {
            total++;
        } else {
            break;
        }
    }

    *size = acc*2;
    stack = (work_t*) calloc (*size, sizeof(work_t));
    *top = 0;
    printf("Got %d taskers, %d possibilities, Starting on house %d\n", ntasks, acc, total);

    // Explore to depth
    while (1) {
        while (pos < total && gMOAS->known[pos / gMOAS->n][pos % gMOAS->n]) {
            ++pos;
        }

        if (pos >= total) {

              // save this history 
              //print_history(mask->history, mask->history_len);
              copy_history(&stack[*top]);
              (*top)++;

              // backtrack
              pos = mask->history[mask->history_len - 1].x * gMOAS->n +
                    mask->history[mask->history_len - 1].y;

              remove_last_from_history();
        }

        if (advance_cell(pos / gMOAS->n, pos % gMOAS->n)) {
            ++pos;
            
        } else {
            if (mask->history_len == 0) {
                break;
            }
            pos = mask->history[mask->history_len - 1].x * gMOAS->n +
                  mask->history[mask->history_len - 1].y;
            remove_last_from_history();
        }
    }

    return stack;
}


int round_robin(int *slaves_state, int ntasks, int state, int begin) {
  int id, robin = begin;

  for (id = 1; id < ntasks; ++id) {
      robin = (robin == ntasks-1) ? (1) : (robin+1);

      if (slaves_state[robin] == state) {

          return robin;
      }
  }

  return -1;
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
            set_cell(i, j, num);
            gMOAS->known[i][j] = num;
        }
    }

    //MPI_Bcast(&gMOAS->n_empty, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //gMOAS->mask->history = calloc(gMOAS->n_empty, sizeof(info_t));
    //gMOAS->mask->history_len = 0;
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
            set_cell(i, j, num);
            gMOAS->known[i][j] = num;
            if (num != 0) {
                gMOAS->n_empty--;
            }
            MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }
    }

    // ?????
    //MPI_Bcast(&gMOAS->n_empty, 1, MPI_INT, 0, MPI_COMM_WORLD);
    gMOAS->mask->history = calloc(gMOAS->n_empty, sizeof(info_t));
    gMOAS->mask->history_len = 0;
    fclose(sudoku_file);
}


void send_work(work_t *work, int id) {

    int size = work->history_len, i;
    printf("Sending size %d\n", size);
    MPI_Send(&size, 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD);

    for(i = 0; i< size; i++) {
        MPI_Send(&(work->history[i].x), 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD);
        MPI_Send(&(work->history[i].y), 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD);
        MPI_Send(&(work->history[i].v), 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD);
    }
}

work_t *receive_work(int id, int size) {
    work_t *work;
    int i, n;
    MPI_Status status;

    work = (work_t*) malloc (sizeof(work_t));
    work->history = (info_t *)calloc(size, sizeof(info_t));
    work->history_len = size;
    for(i=0; i<size; i++) {
        MPI_Recv(&n, 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD, &status);
        work->history[i].x = n;
        MPI_Recv(&n, 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD, &status);
        work->history[i].y = n;
        MPI_Recv(&n, 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD, &status);
        work->history[i].v = n;
        printf("history[%d] = (%d, %d, %d)\n", i, work->history[i].x, work->history[i].y, work->history[i].v);
    }
    return work;
}


void master(const char * filename) {

    MPI_Status status;
    int ntasks, msg, slave;
    int robin = 1;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
    int idle_slaves = ntasks;
    int slaves_state[ntasks];
    work_t *stack, *work;
    int top, wk_size, lost_work = 0;

    read_file(filename, ntasks);
    // Prepare initial work pool
    stack = initial_work(ntasks, &top, &wk_size);
    print_work_stack(ntasks, stack);

    // Distribute initial work
    for (slave = 1; slave < ntasks; ++slave) {
        // get work from work pool
        work = &(stack[--top]);

        // send work
        send_work(work, slave);
        printf("Master sent work %d to Process %d\n", top, slave);

        slaves_state[slave] = 1;
        idle_slaves--;
    }

    /*
    while (1) {

        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        slave = status.MPI_Status;

        if (status.MPI_TAG == NO_WORK_TAG)

            if (top > 0)
                // There is available work
                play = work[--top];

                // Send work
                send_work(&play, slave);

                // If Redistribute
                #ifdef REDIST_ON
                if (top < WORK_TRESH) {
                    robin = round_robin(slaves_state, ntasks, WORKING, slave);

                    if (robin != -1) {
                        MPI_Send(&msg, 1, MPI_INT, robin, RED_TAG, MPI_COMM_WORLD);
                        slaves_state[robin] = REQUEST;
                    }
                    else {
                        lost_work++;

                        robin = 1;
                    }
                }
                #endif

            }
            else {
                // There is no work available
                idle++;
                slaves_state[slave] = IDLE;

                if (idle == ntasks-1) {
                    // No work here, no slaves working, its vacation time!

                    // No solution found

                    exit_colony(ntasks);
                    return;
                }

                #ifdef REDIST_ON
                // Redistribute (if has not passed threshold)
                if (top < WORK_TRESH) {
                    robin = round_robin(slaves_state, ntasks, WORKING, slave);

                    if (robin != -1) {

                        MPI_Send(&msg, 1, MPI_INT, robin, RED_TAG, MPI_COMM_WORLD);
                        slaves_state[robin] = REQUEST;
                    }
                    else {
                        lost_work++;

                        robin = 1;
                    }
                }
                #endif
            }
        }
        else if(status.MPI_TAG == SOLUTION_TAG) {

            // Sends DIE_TAG by broadcast
            exit_colony(ntasks);
        }
        else if(status.MPI_TAG == WORK_TAG) {
            play = msg;

            // Handle work redistributed
            if (idle > 0) {
                // Find one idle slave
                robin = round_robin(slaves_state, ntasks, WORKING, slave);

                // if (robin == -1) ???

                // Send work
                send_work(&play, robin);

                slaves_state[robin] = WORKING;
                idle--;
            }
            else {
                // Add to work queue

                work[top++] = play;
            }

            // Slave that shared work state.
            if (lost_work > 0) {
                lost_work--;

                MPI_Send(&msg, 1, MPI_INT, slave, RED_TAG, MPI_COMM_WORLD);
                slaves_state[slave] = REQUEST;
            } else {

                slaves_state[slave] = WORKING;
            }
        }

    }*/
    exit_colony(ntasks);

}


void slave(int my_id) {

    MPI_Status status;
    MPI_Request request;
    int msg = 0, top, *play;
    int i, flag = -1, state = IDLE, size;
    int init_root, root, init_pos, pos, res, history_len;
    work_t *work;

    // Receive map
    build_map();
    //print_grid();

    while (1) {

        if(flag != 0) {
            MPI_Irecv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
            flag = 0 ;
        }

        MPI_Test(&request, &flag, &status);

        if(flag == 1) {
            if (status.MPI_TAG == WORK_TAG) {

                printf("Process %d got work of size %d\n", my_id, msg);
                work = receive_work(0, msg);
                if(my_id == 2) {
                    print_history(work->history, work->history_len);
                }
                // Sets state to the beggining of history
                restore_from_history(work->history, work->history_len);
                init_root = work->history_len - 1;
                init_pos = gMOAS->mask->history[init_root].x * gMOAS->n + gMOAS->mask->history[init_root].y + 1;

                state = WORKING;
            }
            else if (status.MPI_TAG == RED_TAG && state == WORKING) {
                /*
                // Redistribute work and send to master
                // find current root
                root = root_history(init_root);

                if (((gMOAS->n)*(gMOAS->n) - root) < DEPTH_TRESH) {

                    state = WORKING;
                }
                else if(root != -1) {

                    state = REQUEST;
                }
                else {
                    // make copy of history until root_history

                    // send work to master
                    send_work(&play, 0);
                    // change history
                    rewrite_history(root);

                    printf("Process %d redistributed work\n", my_id);
                    state = WORKING;
                }
                */
                }

                if (root != -1 && ((gMOAS->n)*(gMOAS->n) - root) > DEPTH_TRESH) {

            }
            else if (status.MPI_TAG == SOLUTION_TAG) {
                printf("Process %d found solution\n", my_id);
                return;
            }
            else if (status.MPI_TAG == DIE_TAG) {
                printf("Process %d DIED\n", my_id);
                return;
            }
            flag = -1;
        }

        if(state != IDLE) {
            /*
            // Do work

            res = solve_nsteps(init_pos, &pos, gMOAS->n * gMOAS->n, STEP_SIZE);


            if (res == 0) {
                state = IDLE;

                if (gDONE) {
                    // Send msg SOLUTION_TAG to master

                    // Send history
                }
                else {
                    // Send msg NO_WORK_TAG to master

                }

                clear_history();
            }
            */

        }

        if (state == REQUEST) {
            /*
                // Redistribute work and send to master
                // find current root
                root = root_history(init_root);

                if (((gMOAS->n)*(gMOAS->n) - root) < DEPTH_TRESH) {

                    state = WORKING;
                }
                else if(root != -1) {

                    state = REQUEST;
                }
                else {
                    // make copy of history until root_history

                    // send work to master
                    send_work(&play, 0);
                    // change history
                    rewrite_history(root);

                    printf("Process %d redistributed work\n", my_id);
                    state = WORKING;
                }
                */
        }
    }
}

int main(int argc, char *argv[]) {

    int my_id;
    if(argc < 2) {
        printf("Usage: ./program [filename]\n");
        exit(-1);
    }
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
