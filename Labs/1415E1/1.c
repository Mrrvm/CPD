#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 10

int main(int argc, char const *argv[]) {
    omp_set_num_threads(8);
    int i, M, j;
    int a[100][1000] = {0};

    srand(time(NULL));
    #pragma omp parallel for
    for (i = 0; i < N; i++) {
        printf("Dealing with %d on thread %d \n", i, omp_get_thread_num());

        M = rand() % 1000;
        for (j = 0; j < M; j++)
            #pragma omp atomic
            a[i][j] *= 2.0 + a[i][j];
    }
    return 0;
}
