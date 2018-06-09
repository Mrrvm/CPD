#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1000

int main(int argc, char const *argv[]) {
    omp_set_num_threads(8);
    int i;
    int a[N] = {0};
    int value = 0;
    int found = 0;

    #pragma omp parallel for
    for (i = 0; i < N; i++)
        if (a[i] == value) {
            #pragma omp atomic
            found = found + 1;
            printf("found so far: %i values\n", found);
        }

    return 0;
}
