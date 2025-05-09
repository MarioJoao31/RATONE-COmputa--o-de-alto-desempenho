
#include <stdio.h>

#include <omp.h>


int isPrime(int number) { 

    if (number < 2) return 0;
    
    for (int i=2; i<number; i++)
        
        if (number % i == 0) 
            return 0;
    
    return 1;
}


int main(int argc, char** argv) {

    int N;
    int contaPrimos = 0;
    
    printf("Calculo de intervalo de primos - paralelo 2a\n");
    
    N = 1000000;
    
    printf("Tamanho do intervalo: 1 .. %d\n", N);
    
    #pragma omp parallel
    {
        //contagem de primos por thread
        int aux = 0;
        //agrupar os números em blocos
        int bloco = N / omp_get_num_threads() ;
        //identificar a thread
        int t = omp_get_thread_num();
/*        
        t0: 0*bloco +1 -> 1*bloco
        t1: 1*bloco +1 -> 2* bloco
        t2: 2*bloco -> 3* bloco
        t3: 3*bloco -> N
*/        
        int inicio = t*bloco +1;
        int fim = (t+1)*bloco;
        
        if (t+1 == omp_get_num_threads()) fim = N;
        
        printf("Thread %d inicio %d fim %d\n", t, inicio, fim);
        
        for(int i=inicio; i<=fim; i++) {
            if( isPrime(i) ) 
                aux++;
        }
        
        #pragma omp critical
        contaPrimos += aux;
    } // fim parallel
    
    printf("Contagem de primos: %d\n\n", contaPrimos);

    puts("Valor fixo para 100000 números contém 9592 primos.");
    puts("Valor fixo para 1000000 números contém 78498 primos.");

    return (0);
}

