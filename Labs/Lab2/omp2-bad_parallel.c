/*
DESCRIPTION:
    Parallelizing an inner loop with dependences

    for (iter=0; iter<numiter; iter++) {
      for (i=0; i<size-1; i++) {
        V[i] = f( V[i], V[i+1] );
      }
    }

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define TOTALSIZE 10
#define NUMITER 5

/*
 * * DUMMY FUNCTION
 * */
#define f(x,y) ((x+y)/2.0)


/* MAIN: PROCESS PARAMETERS */
int main(int argc, char *argv[]) {

  /* VARIABLES */
  int i, iter;

  /* DECLARE VECTOR AND AUX DATA STRUCTURES */
  double *V = (double *) malloc(TOTALSIZE * sizeof(double));

  /* 1. INITIALIZE VECTOR */
  for(i = 0; i < TOTALSIZE; i++) {
    V[i]= 0.0 + i;
  }

  /* 2. ITERATIONS LOOP */
  for(iter = 0; iter < NUMITER; iter++) {
    printf("Iteration %d\n", iter);

#pragma omp parallel private(i) 
    /* 2.1. PROCESS ELEMENTS */
    for(i = 0; i < TOTALSIZE-1; i++) {
      V[i] = f(V[i], V[i+1]);
    }
    
    /* 2.2. END ITERATIONS LOOP */
  }

  /* 3. OUTPUT FINAL VALUES */
  printf("Output:\n"); 
  for(i = 0; i < TOTALSIZE; i++) {
    printf("%4d %f\n", i, V[i]);
  }

}
