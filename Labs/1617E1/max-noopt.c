#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_N 2000000

int max(int a[], int N) {
    int i, m = a[0];
    #pragma omp parallel for
    for (i = 0; i < N; i++)

        #pragma omp critical
    {
        if (a[i] > m) {
            m = a[i];
        }
    }
}

int main(int argc, char const *argv[]) {
    srand(time(NULL)); // should only be called once
    omp_set_num_threads(4);

    int a[MAX_N];
    for (int i = 0; i < MAX_N; i++) {
        a[i] = rand() % MAX_N;
    }

    int m = max(a, MAX_N);
    printf("Max Value: %d\n", m);
    return 0;
}
