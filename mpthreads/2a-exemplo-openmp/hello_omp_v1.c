/*
  Compilar sem usar OpenMP, e com OpenMP, executar ambas versões.
  "-fopen" no gcc
 */

#include <stdio.h>

int main() {
       
    #pragma omp parallel
    printf("Esta é uma thread\n");
    
    printf("Fim\n");

    return 0;
}