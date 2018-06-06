#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

static long num_steps = 100000000;
double step;

int main(int argc, char const *argv[]) {
    int i;
    double x, pi, sum = 0.0;

    step = 1.0 / ((double)num_steps);

    int num = omp_get_num_procs();
    printf("Num: %d\n", num);

    #pragma omp parallel for reduction(+ : sum) private(x)
    for (i = 0; i < num_steps; i++) {
        x = (i + 0.5) * step;
        sum += +4.0 / (1.0 + x * x);
    }
    pi = step * sum;
    printf("%f\n", pi);
    return 0;
}
