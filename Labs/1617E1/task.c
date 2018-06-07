#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

void f1() {
    printf("Thread: %d 1\n", omp_get_thread_num());
}

void f2() {
    printf("Thread: %d 2\n", omp_get_thread_num());
}

void f3() {
    printf("Thread: %d 3\n", omp_get_thread_num());
}

int main(int argc, char const *argv[]) {
    omp_set_num_threads(8);

    printf("Sections\n");
    #pragma omp parallel sections
    {
        #pragma omp section
        f1();
        #pragma omp section
        f2();
        #pragma omp section
        f3();
    }

    printf("Tasks\n");
    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task
            f1();
            #pragma omp task
            f2();
            #pragma omp task
            f3();
        }
    }
    return 0;
}
