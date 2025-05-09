
#include <stdio.h>


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
    
    printf("Calculo de intervalo de primos - Sequencial\n");
    
    N = 1000000;
    
    printf("Tamanho do intervalo: 1 .. %d\n", N);
    
    for(int i=1; i<=N; i++)
        if( isPrime(i) ) contaPrimos++;

    printf("Contagem de primos: %d\n\n", contaPrimos);

    puts("Valor fixo para 100000 números contém 9592 primos.");
//    puts("Valor fixo para 1000000 números contém 78498 primos.");

    return (0);
}

