/*
    Sequential Exhaustive Job-Shop Scheduler (Full Search + Verbose Output to TXT)

    ✅ Funcionalidades:
    - Percorre todas as alternativas (sem poda - sem branch-and-bound);
    - Imprime todos os branches explorados (com info de job, operação, tempo, etc);
    - Guarda esses branches num ficheiro "branches.txt";
    - Mantém o makespan ótimo encontrado;

    📄 Compilar:
    gcc -Wall -g -o main_seq_verbose.exe mainSequentialFullSearchVerbose.c

    🚀 Executar:
    ./main_seq_verbose.exe la01.jss output.txt 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <time.h>

#define MAX_JOBS     10
#define MAX_OPS      10
#define MAX_MACHINES 10
#define MAX_REPEATS  100

// Estrutura para representar uma operação do job
typedef struct {
    int machine;
    int duration;
    int start;
    int end;
} Operation;

int num_jobs, num_machines, num_ops;
Operation ops_backup[MAX_JOBS][MAX_OPS];
Operation best_schedule[MAX_JOBS][MAX_OPS];
int best_makespan = INT_MAX;
double program_start_time;
volatile sig_atomic_t interrupted = 0;

unsigned long long branch_count = 0;
int verbose_mode = 1; // Define 1 para ativar output dos branches
FILE *branch_log_fp = NULL; // ficheiro para guardar os branches

// Tratador de interrupção (Ctrl+C)
void handle_interrupt(int signum) {
    interrupted = 1;
    double elapsed = (clock() - program_start_time) / CLOCKS_PER_SEC;
    fprintf(stderr, "\n[INTERRUPT] Melhor makespan: %d | Tempo: %.2f s\n", best_makespan, elapsed);
    if (branch_log_fp) fclose(branch_log_fp);
    exit(EXIT_FAILURE);
}

// Lê ficheiro de input .jss
void read_input(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("Erro ao abrir ficheiro"); exit(EXIT_FAILURE); }
    if (fscanf(fp, "%d %d", &num_jobs, &num_machines) != 2) {
        fprintf(stderr, "Formato inválido\n"); fclose(fp); exit(EXIT_FAILURE);
    }
    num_ops = num_machines;
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            if (fscanf(fp, "%d %d", &ops_backup[j][i].machine, &ops_backup[j][i].duration) != 2) {
                fprintf(stderr, "Erro leitura de operações\n"); fclose(fp); exit(EXIT_FAILURE);
            }
        }
    }
    fclose(fp);
}

// Cópia de um calendário para outro
void copy_schedule(Operation dest[MAX_JOBS][MAX_OPS], Operation src[MAX_JOBS][MAX_OPS]) {
    for (int j = 0; j < num_jobs; j++)
        for (int i = 0; i < num_ops; i++)
            dest[j][i] = src[j][i];
}

// Algoritmo de busca exaustiva (sem poda)
void full_search(int scheduled_ops, int current_makespan,
                 int job_progress[MAX_JOBS],
                 int job_ready[MAX_JOBS],
                 int machine_ready[MAX_MACHINES],
                 Operation current_schedule[MAX_JOBS][MAX_OPS]) {

    if (interrupted) return;

    // Caso base: todas operações foram agendadas
    if (scheduled_ops == num_jobs * num_ops) {
        if (current_makespan < best_makespan) {
            best_makespan = current_makespan;
            copy_schedule(best_schedule, current_schedule);
        }
        return;
    }

    // Para cada job, tenta agendar a próxima operação possível
    for (int j = 0; j < num_jobs; j++) {
        int next_op = job_progress[j];
        if (next_op >= num_ops) continue;

        int m = ops_backup[j][next_op].machine;
        int d = ops_backup[j][next_op].duration;
        int start = machine_ready[m] > job_ready[j] ? machine_ready[m] : job_ready[j];
        int end = start + d;

        // Faz cópias temporárias do estado atual
        Operation temp_schedule[MAX_JOBS][MAX_OPS];
        int temp_job_ready[MAX_JOBS];
        int temp_machine_ready[MAX_MACHINES];
        int temp_job_progress[MAX_JOBS];

        copy_schedule(temp_schedule, current_schedule);
        memcpy(temp_job_ready, job_ready, sizeof(temp_job_ready));
        memcpy(temp_machine_ready, machine_ready, sizeof(temp_machine_ready));
        memcpy(temp_job_progress, job_progress, sizeof(temp_job_progress));

        // Agendamento da operação
        temp_schedule[j][next_op] = (Operation){m, d, start, end};
        temp_machine_ready[m] = end;
        temp_job_ready[j] = end;
        temp_job_progress[j]++;

        // Verbose: imprime o branch atual e salva no ficheiro
        if (verbose_mode) {
            branch_count++;
            fprintf(branch_log_fp,
                    "[Branch %llu | Profundidade %d] Job %d - Op %d | Máquina: %d | Início: %d | Fim: %d | Makespan parcial: %d\n",
                    branch_count, scheduled_ops + 1, j, next_op, m, start, end,
                    (end > current_makespan ? end : current_makespan));
        }

        // Chamada recursiva
        full_search(scheduled_ops + 1,
                    (end > current_makespan ? end : current_makespan),
                    temp_job_progress,
                    temp_job_ready,
                    temp_machine_ready,
                    temp_schedule);
    }
}

// Gantt Chart textual
void print_gantt_chart(FILE *fp) {
    const int block_size = 5;
    int makespan = best_makespan;
    int blocks = (makespan + block_size - 1) / block_size;

    fprintf(fp, "\n# Gantt Chart\n");
    for (int m = 0; m < num_machines; m++) {
        fprintf(fp, "Máquina %2d |", m);
        for (int b = 0; b < blocks; b++) {
            int t_start = b * block_size;
            int t_end = t_start + block_size;
            int printed = 0;
            for (int j = 0; j < num_jobs && !printed; j++)
                for (int i = 0; i < num_ops; i++)
                    if (best_schedule[j][i].machine == m &&
                        best_schedule[j][i].start < t_end &&
                        best_schedule[j][i].end > t_start) {
                        fprintf(fp, "J%d", j);
                        printed = 1;
                        break;
                    }
            if (!printed) fprintf(fp, "  ");
        }
        fprintf(fp, "|\n");
    }
}

// Guarda makespan final e performance
void write_output(const char *filename, double avg_time, int repeats, const char *input_name) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("Erro output"); exit(EXIT_FAILURE); }

    fprintf(fp, "# Solução Job-Shop: %s\n", input_name);
    fprintf(fp, "Melhor makespan: %d\n", best_makespan);

    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++)
            fprintf(fp, "%d ", best_schedule[j][i].start);
        fprintf(fp, "\n");
    }

    print_gantt_chart(fp);
    fprintf(fp, "\n# Performance: Média de %.6f s em %d repetições\n", avg_time, repeats);
    fclose(fp);
}

// Mede o tempo de execução médio
double measure_execution(int repeats) {
    double total = 0.0;
    for (int r = 0; r < repeats; r++) {
        best_makespan = INT_MAX;
        double t0 = (double) clock() / CLOCKS_PER_SEC;

        int job_progress[MAX_JOBS] = {0};
        int job_ready[MAX_JOBS] = {0};
        int machine_ready[MAX_MACHINES] = {0};
        Operation current_schedule[MAX_JOBS][MAX_OPS] = {{{0}}};

        full_search(0, 0, job_progress, job_ready, machine_ready, current_schedule);

        double t1 = (double) clock() / CLOCKS_PER_SEC;
        total += (t1 - t0);
    }
    return total / repeats;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s input.jss output.txt repeticoes\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_interrupt);
    program_start_time = clock();
    read_input(argv[1]);

    // Abre ficheiro para branches
    branch_log_fp = fopen("branches.txt", "w");
    if (!branch_log_fp) { perror("Erro branches.txt"); exit(EXIT_FAILURE); }

    int repeats = atoi(argv[3]);
    if (repeats < 1 || repeats > MAX_REPEATS) {
        fprintf(stderr, "Repetições inválidas\n"); return EXIT_FAILURE;
    }

    double avg_time = measure_execution(repeats);
    write_output(argv[2], avg_time, repeats, argv[1]);

    fclose(branch_log_fp);
    return EXIT_SUCCESS;
}
