#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_JOBS 3
#define MAX_OPS  3
#define NUM_MACHINES 3
#define NUM_THREADS 4  // Definir o número de threads

typedef struct {
    int machine;
    int proc_time;
} Alternative;

typedef struct {
    Alternative *alts;
    int num_alts;
} Operation;

typedef struct {
    Operation ops[MAX_OPS];
    int num_ops;
} Job;

Job jobs[NUM_JOBS];
int best_makespan = __INT_MAX__;
pthread_mutex_t best_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int thread_id;
} ThreadArg;

int compute_makespan(int assignment[NUM_JOBS][MAX_OPS]) {
    int machine_avail[NUM_MACHINES] = {0};
    int job_finish[NUM_JOBS] = {0};
    
    for (int j = 0; j < NUM_JOBS; j++) {
        int time = 0;
        for (int o = 0; o < jobs[j].num_ops; o++) {
            Alternative alt = jobs[j].ops[o].alts[assignment[j][o]];
            int start = time;
            if (machine_avail[alt.machine] > start)
                start = machine_avail[alt.machine];
            int finish = start + alt.proc_time;
            machine_avail[alt.machine] = finish;
            time = finish;
        }
        job_finish[j] = time;
    }
    
    int makespan = 0;
    for (int j = 0; j < NUM_JOBS; j++) {
        if (job_finish[j] > makespan)
            makespan = job_finish[j];
    }
    return makespan;
}

void *thread_func(void *arg) {
    ThreadArg *targ = (ThreadArg*) arg;
    int local_best_makespan = __INT_MAX__;
    int local_assignment[NUM_JOBS][MAX_OPS];

    for (int i = targ->thread_id; i < NUM_JOBS; i += NUM_THREADS) {
        for (int j = 0; j < jobs[i].num_ops; j++) {
            for (int k = 0; k < jobs[i].ops[j].num_alts; k++) {
                local_assignment[i][j] = k;
                int makespan = compute_makespan(local_assignment);
                if (makespan < local_best_makespan) {
                    local_best_makespan = makespan;
                }
            }
        }
    }
    
    pthread_mutex_lock(&best_mutex);
    if (local_best_makespan < best_makespan) {
        best_makespan = local_best_makespan;
    }
    pthread_mutex_unlock(&best_mutex);
    
    free(targ);
    return NULL;
}

void init_instance() {
    for (int i = 0; i < NUM_JOBS; i++) {
        jobs[i].num_ops = MAX_OPS;
        for (int j = 0; j < MAX_OPS; j++) {
            jobs[i].ops[j].num_alts = NUM_MACHINES;
            jobs[i].ops[j].alts = malloc(sizeof(Alternative) * NUM_MACHINES);
            for (int k = 0; k < NUM_MACHINES; k++) {
                jobs[i].ops[j].alts[k].machine = k;
                jobs[i].ops[j].alts[k].proc_time = rand() % 10 + 1;
            }
        }
    }
}

int main(void) {
    clock_t start_time, end_time;
    double cpu_time_used;
    
    init_instance();
    
    start_time = clock();
    
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        ThreadArg *arg = malloc(sizeof(ThreadArg));
        arg->thread_id = i;
        pthread_create(&threads[i], NULL, thread_func, arg);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    end_time = clock();
    cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("Melhor makespan encontrado = %d\n", best_makespan);
    printf("Tempo de execução: %f segundos\n", cpu_time_used);
    
    return 0;
}
