#include <omp.h>
#include <stdio.h>
int main(int argc, char *argv[]) {
    omp_set_num_threads(6);
    int i, v[10];
    #pragma omp parallel for schedule(static, 1)
    for (i = 0; i < 20; i++)
        v[omp_get_thread_num()] = i;
    #pragma omp parallel
    #pragma omp single
    for (i = 0; i < omp_get_num_threads(); i++)
        printf("%d\n", v[i]);
    return 0;
}
