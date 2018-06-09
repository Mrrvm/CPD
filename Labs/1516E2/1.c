#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 100

int main(int argc, char const *argv[]) {
    omp_set_num_threads(8);
    int i, sum = 0;
    int a[N] = {0};

    srand(time(NULL));

    for (int i = 0; i < N; i++)
        a[i] = i;

    #pragma omp parallel for
    for (i = 0; i < N; ++i)
        sum += a[i];

    printf("sum: %d\n", sum);
    return 0;
}
