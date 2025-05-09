#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define NUM_JOBS 100
#define MAX_OPS  100
#define NUM_MACHINES 10
#define NUM_THREADS 6  

#TODO: professor não quer apontadores, nao quer estruturas dinamicas 
#TODO: Mencionar no codigo onde estão as condições de corrida para depois conseguir explicar 
#TODO: uma ideia para fazer Paralelo: Para cada Maquina Usar o mutex, então assim so uma thread pode aceder a essa maquina de cada vez
#TODO: regras: não pode haver duas operações a correr na mesma máquina ao mesmo tempo / a operacao a seguir so pode começar quando a anterior acabar / tem de dar sempre menos que o makespan

#TODO: brench and bound ou Shitf and Bottleneck

#TODO: Usar esta estrutura de dados para representar o problema:
# operacao tem de ter Maquina[start,Duracação,End]
#       operacao 1  | Operacoes 2 |
#job 1   | Maquina  | Maquina    |
#job 2   | Maquina  | Maquina    |
#job 3   | Maquina  | Maquina    |
#job 4   | Maquina  | Maquina    |



int compute_makespan(int assignment[NUM_JOBS][MAX_OPS]) {
    int machine_avail[NUM_MACHINES] = {0};
    int job_finish[NUM_JOBS] = {0};

    for (int j = 0; j < NUM_JOBS; j++) {
        int time = 0;
        for (int o = 0; o < jobs[j].num_ops; o++) {
            int selected_alt = assignment[j][o];
            if (selected_alt < 0 || selected_alt >= jobs[j].ops[o].num_alts) continue;  
            Alternative alt = jobs[j].ops[o].alts[selected_alt];

            int start = (machine_avail[alt.machine] > time) ? machine_avail[alt.machine] : time;
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

void solve_parallel() {
    #pragma omp parallel num_threads(NUM_THREADS)
    {
        int (*local_assignment)[MAX_OPS] = malloc(NUM_JOBS * sizeof(*local_assignment));  
        if (!local_assignment) {
            printf("Erro ao alocar memória\n");
            exit(1);
        }

        #pragma omp for
        for (int i = 0; i < NUM_JOBS; i++) {
            for (int j = 0; j < jobs[i].num_ops; j++) {
                for (int k = 0; k < jobs[i].ops[j].num_alts; k++) {
                    local_assignment[i][j] = k;
                    int makespan = compute_makespan(local_assignment);

                    omp_set_lock(&best_lock);
                    if (makespan < best_makespan) {
                        best_makespan = makespan;
                    }
                    omp_unset_lock(&best_lock);
                }
            }
        }

        free(local_assignment);  
    }
}

void init_instance() {
    for (int i = 0; i < NUM_JOBS; i++) {
        jobs[i].num_ops = MAX_OPS;
        for (int j = 0; j < MAX_OPS; j++) {
            jobs[i].ops[j].num_alts = NUM_MACHINES;
            jobs[i].ops[j].alts = malloc(sizeof(Alternative) * NUM_MACHINES);
            if (!jobs[i].ops[j].alts) {
                printf("Erro ao alocar memória\n");
                exit(1);
            }
            for (int k = 0; k < NUM_MACHINES; k++) {
                jobs[i].ops[j].alts[k].machine = k;
                jobs[i].ops[j].alts[k].proc_time = rand() % 10 + 1;
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

    srand(time(NULL));
    init_instance();
    omp_init_lock(&best_lock);

    start_time = clock();
    solve_parallel();
    end_time = clock();

    cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Melhor makespan encontrado = %d\n", best_makespan);
    printf("Tempo de execução: %f segundos\n", cpu_time_used);

    free_instance();
    omp_destroy_lock(&best_lock);

    return 0;
}
