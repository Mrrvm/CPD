#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void hadamard(int **A, int **B, int **C, int N, int M) {
    int i;
    for (i = 0; i < N; ++i) {
        int j;
        #pragma omp parallel for schedule(static, 20)
        for (j = 0; j < M; ++j)
            C[i][j] = A[i][j] * B[i][j];
    }
}

int main(int argc, char const *argv[]) {
    omp_set_num_threads(4);
    int M = 10000;
    int N = 15000;
    int **a = malloc(sizeof(int *) * N);
    int **b = malloc(sizeof(int *) * N);
    int **c = malloc(sizeof(int *) * N);

    srand(time(NULL));
    for (int i = 0; i < N; i++) {
        a[i] = malloc(sizeof(int) * M);
        b[i] = malloc(sizeof(int) * M);
        c[i] = malloc(sizeof(int) * M);

        for (int j = 0; j < M; j++) {
            a[i][j] = rand() % 100;
            b[i][j] = rand() % 100;
        }
    }

    hadamard(a, b, c, N, M);

    return 0;
}
