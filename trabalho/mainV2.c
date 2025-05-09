/*
    Compilation (Mac):
    clang -Xpreprocessor -fopenmp -I$(brew --prefix libomp)/include -L$(brew --prefix libomp)/lib -lomp main.c -o scheduler

    Usage:
    ./scheduler input.jss output.txt num_threads num_repeats

    Constraints:
    - No pointers or dynamic memory
    - No console I/O during timing
    - Includes average runtime over multiple runs
    - Race conditions are mitigated via per-machine OpenMP locks
*/

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

#define MAX_JOBS 100
#define MAX_OPS 100
#define MAX_MACHINES 100
#define MAX_REPEATS 100

// Structure for an operation (no pointers)
typedef struct {
    int machine;
    int duration;
    int start;
    int end;
} Operation;

// Global data (static memory, no dynamic allocation)
int num_jobs;
int num_ops;        // assumed equal to num_machines
int num_machines;
Operation ops[MAX_JOBS][MAX_OPS];
Operation ops_backup[MAX_JOBS][MAX_OPS];

int machine_available[MAX_MACHINES];  // Shared machine availability
int job_available[MAX_JOBS];          // Sequential job readiness
omp_lock_t machine_lock[MAX_MACHINES]; // Per-machine locks for race condition mitigation

// ================== Input/Output ==================
void read_input(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("Error opening input file"); exit(1); }

    fscanf(fp, "%d %d", &num_jobs, &num_machines);
    num_ops = num_machines; // Assumes square matrix (1 op per machine per job)

    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            int m, d;
            fscanf(fp, "%d %d", &m, &d);
            ops_backup[j][i].machine = m;
            ops_backup[j][i].duration = d;
        }
    }

    fclose(fp);
}

void reset_data() {
    // Reset ops and time info to initial state
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            ops[j][i] = ops_backup[j][i];
            ops[j][i].start = 0;
            ops[j][i].end = 0;
        }
    }
}

void write_output(const char *filename, double avg_time, int repeats) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("Error opening output file"); exit(1); }

    int makespan = 0;
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            if (ops[j][i].end > makespan)
                makespan = ops[j][i].end;
        }
    }

    fprintf(fp, "%d\n", makespan);
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            fprintf(fp, "%d ", ops[j][i].start);
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "\n# Performance Analysis\n");
    fprintf(fp, "Average runtime over %d repetitions: %.6f seconds\n", repeats, avg_time);

    fclose(fp);
}

// ================== Sequential Scheduling ==================
void sequential_schedule() {
    for (int m = 0; m < num_machines; m++) machine_available[m] = 0;
    for (int j = 0; j < num_jobs; j++) job_available[j] = 0;

    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            int m = ops[j][i].machine;
            int ready = (machine_available[m] > job_available[j] ? machine_available[m] : job_available[j]);
            ops[j][i].start = ready;
            ops[j][i].end = ready + ops[j][i].duration;
            job_available[j] = ops[j][i].end;
            machine_available[m] = ops[j][i].end;
        }
    }
}

// ================== Parallel Scheduling with OpenMP ==================
void parallel_schedule(int num_threads) {
    for (int m = 0; m < num_machines; m++) {
        machine_available[m] = 0;
        omp_init_lock(&machine_lock[m]);
    }

    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int j = 0; j < num_jobs; j++) {
        int local_available = 0;  // Job-local availability (thread-local)

        for (int i = 0; i < num_ops; i++) {
            int m = ops[j][i].machine;
            int d = ops[j][i].duration;

            // RACE CONDITION NOTE:
            // Without locking, multiple threads could read or update machine_available[m] simultaneously
            omp_set_lock(&machine_lock[m]);

            int ready = (machine_available[m] > local_available ? machine_available[m] : local_available);
            int start = ready;
            int end = start + d;
            machine_available[m] = end;

            omp_unset_lock(&machine_lock[m]);

            ops[j][i].start = start;
            ops[j][i].end = end;
            local_available = end;
        }
    }

    for (int m = 0; m < num_machines; m++) {
        omp_destroy_lock(&machine_lock[m]);
    }
}

// ================== Timing Wrapper ==================
double measure_execution(int threads, int repeats) {
    double total_time = 0.0;
    for (int r = 0; r < repeats; r++) {
        reset_data(); // Important: reinitialize all values
        double start = omp_get_wtime();

        if (threads <= 1) {
            sequential_schedule();
        } else {
            parallel_schedule(threads);
        }

        double end = omp_get_wtime();
        total_time += (end - start);
    }
    return total_time / repeats;
}

// ================== Main ==================
int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s input.jss output.txt num_threads num_repeats\n", argv[0]);
        return 1;
    }

    read_input(argv[1]);

    int threads = atoi(argv[3]);
    int repeats = atoi(argv[4]);

    if (threads < 1 || repeats < 1 || repeats > MAX_REPEATS) {
        fprintf(stderr, "Invalid thread or repeat count. Must be >= 1 and <= %d\n", MAX_REPEATS);
        return 1;
    }

    double avg_time = measure_execution(threads, repeats);
    write_output(argv[2], avg_time, repeats);

    return 0;
}
