#include <stdio.h>
#include <omp.h>

void printthread() {

#ifdef _OPENMP
    int threadid = omp_get_thread_num();
    int numthreads = omp_get_num_threads();
#else
    int threadid = 0;
    int numthreads = 1;
#endif
    
    printf("Esta é a thread %d, num threads %d\n", threadid, numthreads );
    
}

int main() {
    
#ifdef _OPENMP
    omp_set_num_threads(3);
#endif
    
    #pragma omp parallel
    printthread();

    printf("Fim\n");
    return 0;
}