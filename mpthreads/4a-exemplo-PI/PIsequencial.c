
/* 
 * Pi sequencial
 * series Gregory-Leibniz
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef _OPENMP
	#include <omp.h>
	#define getClock() omp_get_wtime()
#else
	#include <time.h>
	#define getClock() ((double)clock() / CLOCKS_PER_SEC)
#endif

int main(int argc, char** argv) {
  
  long i;
  long n;
  int param_iteracoes;
  
  if (argc > 1)
    param_iteracoes = strtol( argv[1], NULL, 10);
  else
    param_iteracoes = 6; 
  
  n = (long) pow( 10, param_iteracoes);
  
  double factor = 1.0;
  double sum = 0.0;
  double pi_approx;
  
  printf("Calculo PI sequencial\n");
  printf("Numero iteracoes: %ld ( %d )\n", n, param_iteracoes);
  
  printf("Inicio dos calculos...\n");
  double tempoinicio = getClock();

  for (i=0; i < n; i++) {
    sum += factor / (2*i+1);
    factor = -factor;
  }
  pi_approx = 4.0 * sum;

  double tempofim = getClock();
  printf("Fim dos calculos\n");
  
  const double realPiValue = 3.141592653589793238;
  printf("Valor real PI:   %.18f\n", realPiValue);
  printf("Valor aprox PI : %.18f\n", pi_approx);
  printf("Erro = %.1e\n", fabs(pi_approx - realPiValue));

  printf("Tempo de execução (s): %.6f\n", tempofim - tempoinicio);
  
  return (EXIT_SUCCESS);
}

