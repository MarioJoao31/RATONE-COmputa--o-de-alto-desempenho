/*
  Compilar usando 3 configs:
  1) directo, sem opções 		- gcc v2.c
  2) com opção biblioteca, sem openmp 	- gcc -lomp v2.c
  3) com opção de openmp 		- gcc -fopenmp v2.c
 */

#include <stdio.h>
#include <omp.h>

int main() {
    
    omp_set_num_threads(3);
    
    #pragma omp parallel
    printf("Esta é a thread %d, num threads %d\n", omp_get_thread_num(), omp_get_num_threads());

    printf("Fim\n");
    return 0;
}