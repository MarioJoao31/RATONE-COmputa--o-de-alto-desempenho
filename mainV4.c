/*
    Job-Shop Scheduler in C (OpenMP) with Shifting Bottleneck Heuristic

    Constraints per Professor:
    1) No pointers or dynamic memory (all data in static arrays)
    3) Parallel idea: one lock (mutex) per machine to prevent concurrent access
    4) Scheduling rules:
       - No two operations on the same machine at the same time
       - Operations within a job respect their sequence: each starts after the previous ends
       - Overall schedule length (makespan) is minimized relative to sequential baseline
*/

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <limits.h>

#define MAX_JOBS     100
#define MAX_OPS      100
#define MAX_MACHINES 100
#define MAX_REPEATS  100

typedef struct {
    int machine;
    int duration;
    int start;
    int end;
} Operation;

int num_jobs, num_ops, num_machines;
Operation ops[MAX_JOBS][MAX_OPS];
Operation ops_backup[MAX_JOBS][MAX_OPS];
int machine_available[MAX_MACHINES];
int job_available[MAX_JOBS];
omp_lock_t machine_lock[MAX_MACHINES];

void read_input(const char *fn) {
    FILE *fp = fopen(fn, "r");
    if (!fp) { perror("open"); exit(1); }
    fscanf(fp, "%d %d", &num_jobs, &num_machines);
    num_ops = num_machines;
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            fscanf(fp, "%d %d", &ops_backup[j][i].machine, &ops_backup[j][i].duration);
        }
    }
    fclose(fp);
}

void reset_data() {
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            ops[j][i] = ops_backup[j][i];
            ops[j][i].start = 0;
            ops[j][i].end = 0;
        }
    }
}

void sequential_schedule() {
    for (int m = 0; m < num_machines; m++) machine_available[m] = 0;
    for (int j = 0; j < num_jobs; j++) job_available[j] = 0;

    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            int m = ops[j][i].machine;
            int ready = (machine_available[m] > job_available[j]) ? machine_available[m] : job_available[j];
            ops[j][i].start = ready;
            ops[j][i].end = ready + ops[j][i].duration;
            job_available[j] = ops[j][i].end;
            machine_available[m] = ops[j][i].end;
        }
    }
}

int compute_makespan() {
    int makespan = 0;
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            if (ops[j][i].end > makespan)
                makespan = ops[j][i].end;
        }
    }
    return makespan;
}

void shifting_bottleneck() {
    int improved = 1;
    while (improved) {
        improved = 0;
        for (int j1 = 0; j1 < num_jobs - 1; j1++) {
            for (int j2 = j1 + 1; j2 < num_jobs; j2++) {
                for (int m = 0; m < num_machines; m++) {
                    reset_data();
                    // swap job j1 and j2 at all operations on machine m
                    for (int i = 0; i < num_ops; i++) {
                        if (ops[j1][i].machine == m && ops[j2][i].machine == m) {
                            Operation temp = ops_backup[j1][i];
                            ops_backup[j1][i] = ops_backup[j2][i];
                            ops_backup[j2][i] = temp;
                        }
                    }
                    sequential_schedule();
                    int new_makespan = compute_makespan();
                    if (new_makespan < compute_makespan()) {
                        improved = 1;
                    } else {
                        // revert the swap
                        for (int i = 0; i < num_ops; i++) {
                            if (ops_backup[j1][i].machine == m && ops_backup[j2][i].machine == m) {
                                Operation temp = ops_backup[j1][i];
                                ops_backup[j1][i] = ops_backup[j2][i];
                                ops_backup[j2][i] = temp;
                            }
                        }
                    }
                }
            }
        }
    }
    reset_data();
    sequential_schedule();
}

void print_gantt_chart(FILE *fp) {
    const int block_size = 5;
    fprintf(fp, "\n# Gantt Chart (Compressed: 1 char = %d time units)\n", block_size);
    int makespan = compute_makespan();
    int blocks = (makespan + block_size - 1) / block_size;

    for (int m = 0; m < num_machines; m++) {
        fprintf(fp, "Machine %2d |", m);
        for (int b = 0; b < blocks; b++) {
            int t_start = b * block_size;
            int t_end = t_start + block_size;
            int printed = 0;
            for (int j = 0; j < num_jobs; j++) {
                for (int i = 0; i < num_ops; i++) {
                    if (ops[j][i].machine == m && ops[j][i].start < t_end && ops[j][i].end > t_start) {
                        fprintf(fp, "J%d", j);
                        printed = 1;
                        break;
                    }
                }
                if (printed) break;
            }
            if (!printed) fprintf(fp, "  ");
        }
        fprintf(fp, "|\n");
    }
    fprintf(fp, "\nTime       ");
    for (int b = 0; b < blocks; b++) {
        int label = b * block_size;
        if (label < 10) fprintf(fp, "  %d", label);
        else if (label < 100) fprintf(fp, " %d", label);
        else fprintf(fp, "%d", label);
    }
    fprintf(fp, " %d\n", makespan);
}

void write_output(const char *filename, double avg_time, int repeats) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("Error opening output file"); exit(1); }
    int makespan = compute_makespan();
    fprintf(fp, "%d\n", makespan);
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            fprintf(fp, "%d ", ops[j][i].start);
        }
        fprintf(fp, "\n");
    }
    print_gantt_chart(fp);
    fprintf(fp, "\n# Performance Analysis\n");
    fprintf(fp, "Average runtime over %d repetitions: %.6f seconds\n", repeats, avg_time);
    fclose(fp);
}

double measure_execution(int threads, int repeats) {
    double total_time = 0.0;
    for (int r = 0; r < repeats; r++) {
        reset_data();
        double t0 = omp_get_wtime();
        shifting_bottleneck();
        double t1 = omp_get_wtime();
        total_time += (t1 - t0);
    }
    return total_time / repeats;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s input.jss output.txt num_threads num_repeats\n", argv[0]);
        return 1;
    }
    read_input(argv[1]);
    int threads = atoi(argv[3]);
    int repeats = atoi(argv[4]);
    if (threads < 1 || repeats < 1 || repeats > MAX_REPEATS) {
        fprintf(stderr, "Invalid parameters. repeats must be 1..%d\n", MAX_REPEATS);
        return 1;
    }
    double avg_time = measure_execution(threads, repeats);
    write_output(argv[2], avg_time, repeats);
    return 0;
}
