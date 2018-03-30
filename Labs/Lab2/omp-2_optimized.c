#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define TOTALSIZE 10
#define NUMITER 5
#define NUM_THREADS 5

#define f(x,y) ((x+y)/2.0)

int main(int argc, char *argv[]) {

  int i, iter, nt = NUM_THREADS;

  double *V = (double *) malloc(TOTALSIZE * sizeof(double));
  double temp;
  int start, end, tid;

  for(i = 0; i < TOTALSIZE; i++) {
    V[i]= 0.0 + i;
  }

  #pragma omp parallel private(iter, i, start, end, tid, temp) 
  {
    tid = omp_get_thread_num();
    start = (TOTALSIZE/nt)*tid;
    end = (tid+1 == nt) ? TOTALSIZE-1 : (TOTALSIZE/nt)*(tid+1);

    for(iter = 0; iter < NUMITER; iter++) {
      temp = V[end];
      #pragma omp barrier
      for(i = start; i < end; i++) {
        V[i] = (i+1 == end) ? f(V[i], temp) : f(V[i], V[i+1]);
        //printf("V[%d] = %f\n", i, V[i]);
      }
      #pragma omp barrier

    }

  }
  printf("Output:\n"); 
  for(i = 0; i < TOTALSIZE; i++) {
    printf("%4d %f\n", i, V[i]);
  }

}
