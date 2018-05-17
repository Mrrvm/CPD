#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define INIT_BUFF 6
#define WORK_TRESH 4

enum tags {INIT_TAG = 1, DIE_TAG, WORK_TAG, NO_WORK_TAG, SOLUTION_TAG};
enum slave_st {IDLE = 100, WORKING, REQUEST};

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

int *initial_work(int ntasks, int *top, int *size) {

    int depth = 1, acc = gMOAS->box_size;
    int *work;

    while (acc < (ntasks + INIT_BUFF) && depth <= gMOAS->box_size)
        acc *= (gMOAS->box_size - depth++);

    *size = acc*2;
    work = (int*) calloc (*size, sizeof(int));
    *top = 0;

    /* Explore to depth */

    /* Add all possible solutions (to depth) to work */


    return work;
}

void master() {

    MPI_Status status;
    int ntasks, id, msg, slave;
    int robin = 1;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
    int idle_slaves = ntasks;
    int slaves_state[ntasks] = {IDLE};
    int *work, top, wk_size, lost_work = 0;
    int play;

    // Prepare initial work pool
    work = initial_work(ntasks, &top, &wk_size);

    // Distribute initial work
    for (id = 1; id < ntasks; ++id) {
        // get work from work pool
        play = work[--top];

        // send work
        MPI_Send(&play, 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD);
        printf("Master sent work %d to Process %d\n", top, id);

        slaves_state[id] = 1;
        idle_slaves--;
    }

    while (1) {

        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        slave = status.MPI_Status;

        if(status.MPI_TAG == NO_WORK_TAG)

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

            // If found solution
            if(solution) {
                // Send msg SOLUTION_TAG to master
            }

            // When ending
            if(ended) {
                // Send msg NO_WORK_TAG to master
                state = IDLE;
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
