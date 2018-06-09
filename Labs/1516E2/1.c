#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
    omp_set_num_threads(2);
    int i;

    #pragma omp parallel for schedule(dynamic, 2)
    for (i = 0; i < 6; i++) {
        printf("%i, %i\n", omp_get_thread_num(), i);
    }
    return 0;
}
