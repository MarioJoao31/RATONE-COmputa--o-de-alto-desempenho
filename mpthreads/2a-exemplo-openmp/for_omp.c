#include <stdio.h>
#include <omp.h>

int main() {
    
    int N = 10;
       
    #pragma omp parallel for
    for(int i=1; i<=N; i++) {
        
        #ifdef _OPENMP
            int threadid = omp_get_thread_num();
            int numthreads = omp_get_num_threads();
        #else
            int threadid = 0;
            int numthreads = 0;
        #endif

        printf("Contador i %d na thread %d, num threads %d\n", i, threadid, numthreads );
    }
    
    printf("Fim\n");

    return 0;
}

/* Resultado de execução

 $ ./a.out                                                                                            
Contador i 7 na thread 2, num threads 4
Contador i 4 na thread 1, num threads 4
Contador i 1 na thread 0, num threads 4
Contador i 9 na thread 3, num threads 4
Contador i 8 na thread 2, num threads 4
Contador i 5 na thread 1, num threads 4
Contador i 2 na thread 0, num threads 4
Contador i 10 na thread 3, num threads 4
Contador i 6 na thread 1, num threads 4
Contador i 3 na thread 0, num threads 4
Fim

 */