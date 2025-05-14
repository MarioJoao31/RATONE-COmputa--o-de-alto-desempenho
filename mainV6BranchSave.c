/*
    Job-Shop Scheduler in C using Parallel Branch and Bound with Backtracking
    This version guarantees optimality for small problem instances.
        
    gcc -fopenmp -Wall -g -o main.exe mainV6BranchSave.c

    .\main.exe ft06.jss teste2.txt 4 1 > log.txt

    Constraints:
    - No pointers or dynamic memory
    - Static arrays only (MAX_JOBS, MAX_OPS, etc.)
    - Computes an optimal schedule via recursive branch-and-bound
    - Includes OpenMP parallelism at the first depth level
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <limits.h>
#include <signal.h>

#define MAX_JOBS     10
#define MAX_OPS      10
#define MAX_MACHINES 10
#define MAX_REPEATS  100

typedef struct {
    int machine;
    int duration;
    int start;
    int end;
} Operation;

int num_jobs, num_machines, num_ops;
Operation ops_backup[MAX_JOBS][MAX_OPS];
int best_makespan = INT_MAX;
int current_best_live = INT_MAX; // Tracks best found during execution
Operation best_schedule[MAX_JOBS][MAX_OPS];
double program_start_time;
volatile sig_atomic_t interrupted = 0;
#pragma omp threadprivate(best_makespan, best_schedule)

void handle_interrupt(int signum) {
    interrupted = 1;
    double elapsed = omp_get_wtime() - program_start_time;
    fprintf(stderr, "\n[INTERRUPTED] Best makespan so far: %d | Total time: %.2f sec\n", current_best_live, elapsed);
    FILE *fp = fopen("interrupted_output.txt", "w");
    if (fp) {
        fprintf(fp, "# INTERRUPTED EXECUTION\n");
        fprintf(fp, "Best makespan: %d\n", current_best_live);
        fprintf(fp, "Total time: %.2f sec\n", elapsed);
        for (int j = 0; j < num_jobs; j++) {
            for (int i = 0; i < num_ops; i++) {
                fprintf(fp, "%d ", best_schedule[j][i].start);
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
    exit(EXIT_FAILURE);
}

void read_input(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("Error opening input file"); exit(EXIT_FAILURE); }
    if (fscanf(fp, "%d %d", &num_jobs, &num_machines) != 2) {
        fprintf(stderr, "Invalid input format\n"); fclose(fp); exit(EXIT_FAILURE);
    }
    num_ops = num_machines;
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            if (fscanf(fp, "%d %d", &ops_backup[j][i].machine, &ops_backup[j][i].duration) != 2) {
                fprintf(stderr, "Invalid operation data\n"); fclose(fp); exit(EXIT_FAILURE);
            }
        }
    }
    fclose(fp);
}

void copy_schedule(Operation dest[MAX_JOBS][MAX_OPS], Operation src[MAX_JOBS][MAX_OPS]) {
    for (int j = 0; j < num_jobs; j++)
        for (int i = 0; i < num_ops; i++)
            dest[j][i] = src[j][i];
}

void branch_and_bound(int scheduled_ops, int current_makespan,
                      int job_progress[MAX_JOBS],
                      int job_ready[MAX_JOBS],
                      int machine_ready[MAX_MACHINES],
                      Operation current_schedule[MAX_JOBS][MAX_OPS]) {
    static unsigned long long step_count = 0;
    if (interrupted) return;

    if (scheduled_ops == num_jobs * num_ops) {
        if (current_makespan < best_makespan) {
            #pragma omp critical
            {
                if (current_makespan < best_makespan) {
                    best_makespan = current_makespan;
                    current_best_live = current_makespan;
                    copy_schedule(best_schedule, current_schedule);
                }
            }
        }
        return;
    }

    for (int j = 0; j < num_jobs; j++) {
        int next_op = job_progress[j];
        if (next_op >= num_ops) continue;

        int m = ops_backup[j][next_op].machine;
        int d = ops_backup[j][next_op].duration;
        int start = machine_ready[m] > job_ready[j] ? machine_ready[m] : job_ready[j];
        int end = start + d;

        if (end >= best_makespan) continue; // prune

        Operation temp_schedule[MAX_JOBS][MAX_OPS];
        int temp_job_ready[MAX_JOBS];
        int temp_machine_ready[MAX_MACHINES];
        int temp_job_progress[MAX_JOBS];

        copy_schedule(temp_schedule, current_schedule);
        memcpy(temp_job_ready, job_ready, sizeof(temp_job_ready));
        memcpy(temp_machine_ready, machine_ready, sizeof(temp_machine_ready));
        memcpy(temp_job_progress, job_progress, sizeof(temp_job_progress));

        temp_schedule[j][next_op].machine = m;
        temp_schedule[j][next_op].duration = d;
        temp_schedule[j][next_op].start = start;
        temp_schedule[j][next_op].end = end;
        temp_machine_ready[m] = end;
        temp_job_ready[j] = end;
        temp_job_progress[j]++;

        #pragma omp atomic
        step_count++;
        if (step_count % 100000000 == 0) {
            double elapsed = omp_get_wtime() - program_start_time;
            printf("[Thread %d] Iteration %llu | Current=%d | Best=%d | Elapsed=%.2fs\n",
                   omp_get_thread_num(), step_count, current_makespan, current_best_live, elapsed);
            fflush(stdout);
        }

        branch_and_bound(scheduled_ops + 1,
                         (end > current_makespan ? end : current_makespan),
                         temp_job_progress,
                         temp_job_ready,
                         temp_machine_ready,
                         temp_schedule);
    }
}

void print_gantt_chart(FILE *fp) {
    const int block_size = 5;
    int makespan = best_makespan;
    int blocks = (makespan + block_size - 1) / block_size;

    fprintf(fp, "\n# Gantt Chart (1 char = %d time units)\n", block_size);
    for (int m = 0; m < num_machines; m++) {
        fprintf(fp, "Machine %2d |", m);
        for (int b = 0; b < blocks; b++) {
            int t_start = b * block_size;
            int t_end = t_start + block_size;
            int printed = 0;
            for (int j = 0; j < num_jobs && !printed; j++)
                for (int i = 0; i < num_ops; i++)
                    if (best_schedule[j][i].machine == m && best_schedule[j][i].start < t_end && best_schedule[j][i].end > t_start) {
                        fprintf(fp, "J%d", j);
                        printed = 1;
                        break;
                    }
            if (!printed) fprintf(fp, "  ");
        }
        fprintf(fp, "|\n");
    }
    fprintf(fp, "\nTime       ");
    for (int b = 0; b < blocks; b++) {
        int label = b * block_size;
        fprintf(fp, label < 10 ? "  %d" : label < 100 ? " %d" : "%d", label);
    }
    fprintf(fp, " %d\n", makespan);
}

void write_output(const char *filename, double avg_time, int repeats, const char *input_name) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("Error opening output file"); exit(EXIT_FAILURE); }

    fprintf(fp, "# Job-Shop Solution for: %s\n", input_name);
    fprintf(fp, "# Jobs: %d | Machines: %d | Operations per Job: %d\n\n", num_jobs, num_machines, num_ops);

    fprintf(fp, "Best makespan: %d\n", best_makespan);
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++)
            fprintf(fp, "%d ", best_schedule[j][i].start);
        fprintf(fp, "\n");
    }
    print_gantt_chart(fp);
    fprintf(fp, "\n# Performance Analysis\n");
    fprintf(fp, "Average runtime over %d repetitions: %.6f seconds\n", repeats, avg_time);
    fclose(fp);
}


double measure_execution(int threads, int repeats) {
    double total = 0.0;
    for (int r = 0; r < repeats; r++) {
        best_makespan = INT_MAX;
        current_best_live = INT_MAX;
        double t0 = omp_get_wtime();

        #pragma omp parallel num_threads(threads)
        {
            #pragma omp for schedule(dynamic)
            for (int seed_job = 0; seed_job < num_jobs; seed_job++) {
                int job_progress[MAX_JOBS] = {0};
                int job_ready[MAX_JOBS] = {0};
                int machine_ready[MAX_MACHINES] = {0};
                Operation current_schedule[MAX_JOBS][MAX_OPS] = {{{0}}};

                int m = ops_backup[seed_job][0].machine;
                int d = ops_backup[seed_job][0].duration;
                current_schedule[seed_job][0].machine = m;
                current_schedule[seed_job][0].duration = d;
                current_schedule[seed_job][0].start = 0;
                current_schedule[seed_job][0].end = d;
                machine_ready[m] = d;
                job_ready[seed_job] = d;
                job_progress[seed_job] = 1;

                branch_and_bound(1, d, job_progress, job_ready, machine_ready, current_schedule);
            }
        }

        double t1 = omp_get_wtime();
        total += (t1 - t0);
    }
    return total / repeats;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s input.jss output.txt threads repeats\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_interrupt);
    program_start_time = omp_get_wtime();
    read_input(argv[1]);

    int threads = atoi(argv[3]);
    int repeats = atoi(argv[4]);

    if (repeats < 1 || repeats > MAX_REPEATS) {
        fprintf(stderr, "Invalid number of repetitions.\n");
        return EXIT_FAILURE;
    }

    double avg_time = measure_execution(threads, repeats);
    write_output(argv[2], avg_time, repeats, argv[1]);
    return EXIT_SUCCESS;
}


// TODO: fazer o paralelo, fazer a arvore full, cada node usa uma thread.
// 