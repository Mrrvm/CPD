#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define TOTALSIZE 10
#define NUMITER 5
#define NUM_THREADS 5

#define f(x,y) ((x+y)/2.0)

int main(int argc, char *argv[]) {

  int i, iter, nt = NUM_THREADS, tid;

  double *V = (double *) malloc(TOTALSIZE * sizeof(double));
  double *V2 = (double *) malloc(TOTALSIZE * sizeof(double));

  for(i = 0; i < TOTALSIZE; i++) {
    V[i]= 0.0 + i;
    V2[i]= 0.0 + i;
  }

  for(iter = 0; iter < NUMITER; iter++) {
      //printf("Iteration %d\n", iter);

      for(i = 0; i < TOTALSIZE; i++) {
        V2[i] = V[i];
      }

      #pragma omp parallel private(i, tid)
      {
        tid = omp_get_thread_num();
        #pragma omp for 
          for(i = 0; i < TOTALSIZE-1; i++) {
            //printf("Thread %d outputs\n\n", tid); fflush(stdout);
            V[i] = f(V2[i], V2[i+1]);
            //printf("%4d %f\n", i, V[i]); fflush(stdout);
          }
      }
  }

  printf("Final Output\n");
  for(i = 0; i < TOTALSIZE; i++) {
    printf("%4d %f\n", i, V[i]);
  }

}
