#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int function(int *a, int N, int value) {
    int retval = 0;
    int i;
    #pragma omp parallel for
    for (i = 0; i < N; i++) {
        if (a[i] == value)
            retval = 1;
    }
    return retval;
}
