#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define INIT_TAG 1
#define DIE_TAG 2
#define WORK_TAG 3
#define NO_WORK_TAG 4
#define PLAY_TAG 5
#define FINISH_TAG 6

void exit_colony(int ntasks) {
    int id, msg = 0;

    for (id = 1; id < ntasks; ++id) {
        MPI_Send(&msg, 1, MPI_INT, id, DIE_TAG, MPI_COMM_WORLD);
    }
}

void master(){

    int ntasks, id, msg;
    int top, active_slaves = 0;
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

    // Prepare initial work pool
    int nwork = 10;

    // Distribute initial work
    for (id = 1; id < ntasks; ++id) {
        // get work from work pool
        top = nwork--;

        // send work
        MPI_Send(&top, 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD);
        printf("Master sent work %d to Process %d\n", top, id);

        active_slaves++;
    }

    // Redistribute work while there is available work
    while (nwork > 0) {
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == NO_WORK_TAG) {
          // get new work from work pool
          top = nwork--;

          // send new work
          MPI_Send(&top, 1, MPI_INT, status.MPI_SOURCE, WORK_TAG, MPI_COMM_WORLD);
        } else if (status.MPI_TAG == FINISH_TAG) {
            printf("Solution! Can exit all\n");

            exit_colony(ntasks);
            return;
        }
    }

    // Wait for still active slaves
    while (active_slaves > 0) {
      MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      if (status.MPI_TAG == NO_WORK_TAG) {

          active_slaves--;
      } else if (status.MPI_TAG == FINISH_TAG) {
          printf("Solution! Can exit all\n");

          exit_colony(ntasks);
          return;
      }
    }

    // Send exit signal
    exit_colony(ntasks);
}

void slave(int my_id) {

    MPI_Status status;
    MPI_Request request;
    int msg = 0, top, *play;
    int i, flag;

    while(1) {
        MPI_Recv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == DIE_TAG) {
            printf("Process %d DIED\n", my_id);

            return;
        }
        else if(status.MPI_TAG == WORK_TAG) {
            top = msg;

            printf("Process %d got work %d\n", my_id, msg);

            MPI_Irecv(&msg, 1, MPI_INT, 0, DIE_TAG, MPI_COMM_WORLD, &request);

            // do work (watch out for order)
            for (i = 0; i < 3; i++) {
                MPI_Test(request, &flag, &status);

                if (flag) {
                  printf("Process %d DIED\n", my_id);

                  return;
                }

                sleep(1);
                printf("Process %d id work %d/%d\n", my_id, i, top);
            }

            MPI_Cancel(request);

            MPI_Test(request, &flag, &status);

            if (flag) {
                printf("Process %d DIED\n", my_id);

                return;
            }

            // final work?
            if (msg == my_id*2) {
                printf("Process %d found solution %d!\n", my_id, top);

                MPI_Send(&msg, 1, MPI_INT, 0, FINISH_TAG, MPI_COMM_WORLD);
            } else {
                printf("Process %d finished work %d\n", my_id, top);

                MPI_Send(&msg, 1, MPI_INT, 0, NO_WORK_TAG, MPI_COMM_WORLD);
            }
        }

        //print_grid();
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
