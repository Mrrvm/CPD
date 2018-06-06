#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int se(int v[], int n) {
    int *m = calloc(n, sizeof(int));
    int i, p = 2, t = 0;
    while (p * p < n) {
        #pragma omp parallel
        {
            #pragma omp for
            for (i = p * p; i < n; i += p)
                m[i] = 1;

            #pragma omp single
            for (p++; m[p]; p++)
                ;
        }
    }
    #pragma omp parallel for
    for (i = 2; i < n; i++)
        if (!m[i]) {
            #pragma omp critical
            v[t++] = i;
        }
    free(m);
    return t;
}

int main(int argc, char const *argv[]) {
    int n = 100;
    int v[100];
    int t = se(v, n);
    omp_set_num_threads(4);
    for (int i = 0; i < t; i++) {
        printf("%d ", v[i]);
    }

    return 0;
}
