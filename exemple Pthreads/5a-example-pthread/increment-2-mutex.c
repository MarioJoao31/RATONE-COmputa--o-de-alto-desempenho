/*
  Exemplo retirado do livro: Introduction to Parallel Programming, Pacheco

  $ gcc -g -Wall -o pth_hello pth_hello.c -lpthread
  $ ./pth hello <number of threads>
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* Global variable: accessible to all threads */ 
int thread_count;
long total_count = 2000000;
long counter = 0;

pthread_mutex_t mutex;


void* Increment(void* rank); /* Thread function */

int main(int argc, char* argv[]) {
  long thread;
  pthread_t* thread_handles;

  /* Get number of threads from command line */
  if (argc < 2) exit(1);
  thread_count = strtol(argv[1], NULL, 10);

  thread_handles = malloc (thread_count * sizeof(pthread_t));

  pthread_mutex_init( &mutex, NULL);

  for (thread = 0; thread < thread_count; thread++) 
    pthread_create(&thread_handles[thread], NULL, Increment, (void*) thread);
  
//  printf("Hello from the main thread\n");

  for (thread = 0; thread < thread_count; thread++) 
    pthread_join(thread_handles[thread], NULL);

  printf("Main thread: total %ld\n", counter);

  free(thread_handles);
  pthread_mutex_destroy( &mutex);

  return 0; 
}

void* Increment(void* rank) { 
  long my_rank = (long) rank;
  long my_share = total_count / thread_count;
  long my_start = my_share * my_rank;
  long my_end = my_start + my_share;
  
  if (my_rank + 1 == thread_count) my_end = total_count;

  printf("Thread %ld of %d, start %ld end %ld\n", my_rank, thread_count, my_start, my_end);
  
  for (long i = my_start; i<my_end; i++) {

    pthread_mutex_lock(&mutex);
    counter++;
    pthread_mutex_unlock(&mutex);
  }

  return NULL; 
} /* Increment */


