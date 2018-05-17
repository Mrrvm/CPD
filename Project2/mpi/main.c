#include <inttypes.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define INIT_BUFF 6
#define WORK_TRESH 4
#define STEP_SIZE 10
#define DEPTH_TRESH 6

enum tags {INIT_TAG = 1, DIE_TAG, WORK_TAG, NO_WORK_TAG, SOLUTION_TAG};
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

void print_grid(mask_t *mask);
void free_gMOAS();
void build_map();

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

int root_history(mask_t *mask, int root) {
    int i;

    /* gets first history whose value is not the last possible */
    for (i = root; i < mask->history_len; i++) {

        if (mask->history[mask->history_len].v != gMOAS->n)
            return i;
    }

    return -1;
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

int solve_nsteps(int root, int *pos, int total, int nsteps) {
    int i;

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
            if (gMOAS->mask->history_len == root) {
                return 0;
            }
            (*pos) = gMOAS->mask->history[gMOAS->mask->history_len - 1].x * gMOAS->n +
                  gMOAS->mask->history[gMOAS->mask->history_len - 1].y;
            remove_last_from_history(gMOAS->mask);
        }
    }

    return 1;
}

work_t *initial_work(int ntasks, int *top, int *size) {

    int depth = 1, acc = gMOAS->box_size;
    work_t *work;
    int pos = 0;

    while (acc < (ntasks + INIT_BUFF) && depth <= gMOAS->box_size)
        acc *= (gMOAS->box_size - depth++);

    *size = acc*2;
    work = (work_t*) calloc (*size, sizeof(work_t));
    *top = 0;

    /* Explore to depth */
    while (1) {
        while (pos < total && gMOAS->known[pos / gMOAS->n][pos % gMOAS->n]) {
            ++pos;
        }
        if (pos >= total) {
              /* save this history */
              work[*top].history =
              work[*top].history_len =
              (*top)++;

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

void exit_colony(int ntasks) {
    int id, msg = 0;

    for (id = 1; id < ntasks; ++id) {
        MPI_Bcast(&msg, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
}

void round_robin(int *slaves_state, int ntasks, int state, int begin) {
  int id, robin = begin;

  for (id = 1; id < ntasks; ++id) {
      robin = (robin == ntasks-1) ? (1) : (robin+1);

      if (slaves_state[robin] == state) {

          return robin;
      }
  }

  return -1;
}

void master() {

    MPI_Status status;
    int ntasks, id, msg, slave;
    int robin = 1;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
    int idle_slaves = ntasks;
    int slaves_state[ntasks] = {IDLE};
    work_t *work, play;
    int top, wk_size, lost_work = 0;

    // Prepare initial work pool
    work = initial_work(ntasks, &top, &wk_size);

    // Distribute initial work
    for (id = 1; id < ntasks; ++id) {
        // get work from work pool
        play = work[--top];

        // send work FIXME (play)
        MPI_Send(&play, 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD);
        printf("Master sent work %d to Process %d\n", top, id);

        slaves_state[id] = 1;
        idle_slaves--;
    }

    while (1) {

        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        slave = status.MPI_Status;

        if (status.MPI_TAG == NO_WORK_TAG)

            if (top > 0)
                /* There is available work */
                play = work[--top];

                /* Send work */
                MPI_Send(&play, 1, MPI_INT, slave, WORK_TAG, MPI_COMM_WORLD);

                /* If Redistribute */
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

            }
            else {
                /* There is no work available */
                idle++;
                slaves_state[slave] = IDLE;

                if (idle == ntasks-1) {
                    /* No work here, no slaves working, its vacation time! */

                    /* No solution found */

                    exit_colony(ntasks);
                    return;
                }

                /* Redistribute (if has not passed threshold) */
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
            }
        }
        else if(status.MPI_TAG == SOLUTION_TAG) {

            // Sends DIE_TAG by broadcast
            exit_colony(ntasks);
        }
        else if(status.MPI_TAG == WORK_TAG) {
            play = msg;

            /* Handle work redistributed */
            if (idle > 0) {
                /* Find one idle slave */
                robin = round_robin(slaves_state, ntasks, WORKING, slave);

                /* if (robin == -1) ????*/

                /* Send work */
                MPI_Send(&play, 1, MPI_INT, robin, WORK_TAG, MPI_COMM_WORLD);

                slaves_state[robin] = WORKING;
                idle--;
            }
            else {
                /* Add to work queue */

                work[top++] = play;
            }

            /* Slave that shared work state. */
            if (lost_work > 0) {
                lost_work--;

                MPI_Send(&msg, 1, MPI_INT, slave, RED_TAG, MPI_COMM_WORLD);
                slaves_state[slave] = REQUEST;
            } else {

                slaves_state[slave] = WORKING;
            }
        }

    }

}


void slave(int my_id) {

    MPI_Status status;
    MPI_Request request;
    int msg = 0, top, *play;
    int i, flag = -1, state = IDLE, size;
    int root, pos, res;

    while (1) {

        if(flag != 0) {
            MPI_Irecv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
            flag = 0 ;
        }

        MPI_Test(&request, &flag, &status);

        if(flag == 1) {
            if (status.MPI_TAG == WORK_TAG) {
                // Probe msg to get size
                MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
                MPI_Get_count(&status, MPI_INT, &size);
                // Alloc work with size
                // Set msg to work data
                printf("Process %d got work %d\n", my_id, msg);
                state = WORKING;
            }
            else if (status.MPI_TAG == RED_TAG) {
                // Redistribute work and send to master
                // find top
                root_history();

                // build history from top

                // change history
                rewrite_history();

                printf("Process %d redistributed work\n", my_id);

                state = WORKING;
            }
            else if (status.MPI_TAG == INIT_TAG) {
                // Receive map

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

        if(state == WORKING) {
            // Do work
            res = solve_nsteps(root, &pos, n->, STEP_SIZE);

            if (res == 0) {
                state = IDLE;

                if (gDONE) {
                    // Send msg SOLUTION_TAG to master

                }
                else {
                    // Send msg NO_WORK_TAG to master

                }
            }

        }
    }
}

int main(int argc, char *argv[]) {

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
