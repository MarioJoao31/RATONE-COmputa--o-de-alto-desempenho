#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_JOBS 100
#define MAX_OPS  100
#define NUM_MACHINES 10

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

int compute_makespan(int assignment[NUM_JOBS][MAX_OPS]) {
    int machine_avail[NUM_MACHINES] = {0};
    int job_finish[NUM_JOBS] = {0};
    
    for (int j = 0; j < NUM_JOBS; j++) {
        int time = 0;
        for (int o = 0; o < jobs[j].num_ops; o++) {
            int selected_alt = assignment[j][o];
            if (selected_alt < 0 || selected_alt >= jobs[j].ops[o].num_alts) {
                continue;  // Evita acessar memória inválida
            }
            Alternative alt = jobs[j].ops[o].alts[selected_alt];
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

void solve_sequential() {
    int assignment[NUM_JOBS][MAX_OPS];

    // Inicializa assignment com valores válidos
    for (int i = 0; i < NUM_JOBS; i++) {
        for (int j = 0; j < MAX_OPS; j++) {
            assignment[i][j] = 0; // Define uma alocação válida inicial
        }
    }
    
    for (int i = 0; i < NUM_JOBS; i++) {
        for (int j = 0; j < jobs[i].num_ops; j++) {
            for (int k = 0; k < jobs[i].ops[j].num_alts; k++) {
                assignment[i][j] = k;
                int makespan = compute_makespan(assignment);
                if (makespan < best_makespan) {
                    best_makespan = makespan;
                }
            }
        }
    }
}

void init_instance() {
    for (int i = 0; i < NUM_JOBS; i++) {
        jobs[i].num_ops = MAX_OPS;
        for (int j = 0; j < MAX_OPS; j++) {
            jobs[i].ops[j].num_alts = NUM_MACHINES;
            jobs[i].ops[j].alts = malloc(sizeof(Alternative) * NUM_MACHINES);
            if (jobs[i].ops[j].alts == NULL) {
                printf("Erro ao alocar memória\n");
                exit(1);
            }
            for (int k = 0; k < NUM_MACHINES; k++) {
                jobs[i].ops[j].alts[k].machine = k;
                jobs[i].ops[j].alts[k].proc_time = rand() % 50 + 1;
            }
        }
    }
}

void free_instance() {
    for (int i = 0; i < NUM_JOBS; i++) {
        for (int j = 0; j < MAX_OPS; j++) {
            free(jobs[i].ops[j].alts);
        }
    }
}

int main(void) {
    clock_t start_time, end_time;
    double cpu_time_used;
    
    srand(time(NULL)); // Garante aleatoriedade nos tempos de processamento
    init_instance();
    
    start_time = clock();
    solve_sequential();
    end_time = clock();
    
    cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("Melhor makespan encontrado = %d\n", best_makespan);
    printf("Tempo de execução: %f segundos\n", cpu_time_used);

    free_instance(); // Libera memória alocada dinamicamente
    
    return 0;
}
