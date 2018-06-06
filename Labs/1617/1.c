#include <omp.h>
#include <stdio.h>
int main(int argc, char *argv[]) {
    int i;
    #pragma omp parallel num_threads(2)
    {
        printf("X");
        #pragma omp for schedule(static, 2)
        for (i = 0; i < 7; i++)
            printf("%d", i);
        #pragma omp single
        printf("Y");
        printf("\n");
    }
}
