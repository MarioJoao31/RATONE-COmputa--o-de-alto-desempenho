
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* Global variable: accessible to all threads */ 
int thread_count;
long total_count = 2000000l;
long counter = 0l;

void* Increment(void* rank); /* Thread function */

int main(int argc, char* argv[]) {
  long thread;
  pthread_t* thread_handles;

  /* Get number of threads from command line */
  if (argc < 2) {fprintf(stderr, "missing argument\n"); exit(1);}

  thread_count = strtol(argv[1], NULL, 10);

  thread_handles = malloc (thread_count * sizeof(pthread_t));

  for (thread = 0; thread < thread_count; thread++) 
    pthread_create(&thread_handles[thread], NULL, Increment, (void*) thread);
  
  printf("Hello from the main thread\n");

  for (thread = 0; thread < thread_count; thread++) 
    pthread_join(thread_handles[thread], NULL);

  printf("Main thread: total %ld\n", counter);

  free(thread_handles);

  return 0; 
}

void* Increment(void* rank) { 
  long my_rank = (long) rank;
  long my_share = total_count / thread_count;

  printf("Thread %ld of %d, counting %ld\n", my_rank, thread_count, my_share);
  
  for (long i = 0; i<my_share; i++) {
    counter++;
  }

  return NULL; 
} /* Increment */


