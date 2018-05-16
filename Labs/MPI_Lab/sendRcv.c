/*
 *   Send / Recv
 *
 *   José Monteiro, DEI / IST
 *
 *   Last modification: 2 November 2014
 */

#include <stdio.h>
#include <mpi.h>

int main (int argc, char *argv[]) {

    MPI_Status status;
    int id, p,
	i, rounds;
    double secs;

    MPI_Init (&argc, &argv);

    MPI_Comm_rank (MPI_COMM_WORLD, &id);
    MPI_Comm_size (MPI_COMM_WORLD, &p);

    if(argc != 2){
    	if (!id) printf ("Command line: %s <n-rounds>\n", argv[0]);
    	MPI_Finalize();
    	exit (1);
    }
    rounds = atoi(argv[1]);

    MPI_Barrier (MPI_COMM_WORLD);
    secs = - MPI_Wtime();

    for(i = 0; i < rounds; i++){
    	if(!id){
    	    MPI_Send(&i, 1, MPI_INT, 1, i, MPI_COMM_WORLD);
    	    MPI_Recv(&i, 1, MPI_INT, p-1, i, MPI_COMM_WORLD, &status);
    	}
    	else{
    	    MPI_Recv(&i, 1, MPI_INT, id-1, i, MPI_COMM_WORLD, &status);
    	    MPI_Send(&i, 1, MPI_INT, (id+1)%p, i, MPI_COMM_WORLD);
    	}
    }

    MPI_Barrier (MPI_COMM_WORLD);
    secs += MPI_Wtime();

    if(!id){
	printf("Rounds= %d, N Processes = %d, Time = %12.6f sec,\n",
	       rounds, p, secs);
	printf ("Average time per Send/Recv = %6.2f us\n",
		secs * 1e6 / (2*rounds*p));
   }
   MPI_Finalize();
   return 0;
}
