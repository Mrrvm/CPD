#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define INIT_TAG 1
#define DIE_TAG 2
#define WORK_TAG 3
#define NO_WORK_TAG 4
#define SOLUTION_TAG 6

#define IDLE 100
#define WORKING 101


void exit_colony(int ntasks) {
    int id, msg = 0;

    for (id = 1; id < ntasks; ++id) {
        MPI_Bcast(&msg, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
}

void master() {

    MPI_Status status;
    int ntasks, id, msg, top;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
    int idle_slaves = ntasks;
    int slaves_state[ntasks] = {0};

    // Prepare initial work pool
    int nwork = 10;

    // Distribute initial work
    for (id = 1; id < ntasks; ++id) {
        // get work from work pool
        top = nwork--;

        // send work
        MPI_Send(&top, 1, MPI_INT, id, WORK_TAG, MPI_COMM_WORLD);
        printf("Master sent work %d to Process %d\n", top, id);

        slaves_state[id] = 1;
        idle_slaves--;
    }

    while (1) {

        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if(status.MPI_TAG == NO_WORK_TAG) 
            // Send work from queue
            // Or,
            // Gets work from other slave (send msg RED_TAG)
        }
        else if(status.MPI_TAG == SOLUTION_TAG) {
            // Sends DIE_TAG by broadcast
            exit_colony(ntasks);
            
        }
        else if(status.MPI_TAG == WORK_TAG) {
            // Saves on queue
            // Or,
            // Sends immediately to the neeedy slave
        }

    }























/*

    // Redistribute work while there is available work
    while (nwork > 0) {
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                 &status);

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
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                 &status);

        if (status.MPI_TAG == NO_WORK_TAG) {

            active_slaves--;
        } else if (status.MPI_TAG == FINISH_TAG) {
            printf("Solution! Can exit all\n");

            exit_colony(ntasks);
            return;
        }
    }
*/

}

void slave(int my_id) {

    MPI_Status status;
    MPI_Request request;
    int msg = 0, top, *play;
    int i, flag = -1, state = IDLE;

    while (1) {

        if(flag != 0) {
            MPI_Irecv(&msg, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
            flag = 0 ;
        }

        MPI_Test(&request, &flag, &status);

        if(flag == 1) {
            if (status.MPI_TAG == WORK_TAG) {
                // Probe msg to get size
                // Receive work 
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
