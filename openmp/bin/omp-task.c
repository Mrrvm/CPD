/*
DESCRIPTION:
 		Experiments with thread synchronization

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NUMITER 3

/* main: process parameters */
int main(int argc, char *argv[]) {

  /* variables */
  int i, tid;

  /* create parallel region, make both variable thread private */
  #pragma omp parallel private(i,tid)
  {
      /* get id of thread */
      tid = omp_get_thread_num();
      printf("Im thread %d\n", tid);
      int j = 0;

      #pragma omp single 
        for(i = 0; i < NUMITER; i++) {
          /* This distributes the tasks per threads */
          #pragma omp task private(j) 
          {
            j=i;
    	      printf("j: %d\n", j);
          }
        }
  }

  /* only master thread prints this one */
  printf("All threads have finished!\n");
}
