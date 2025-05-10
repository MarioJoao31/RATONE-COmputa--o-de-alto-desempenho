/*
    Job-Shop Scheduler in C (OpenMP)

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

#define MAX_JOBS 100     // no dynamic allocation (Constraint 1)
#define MAX_OPS 100      // assumes num_ops == num_machines
#define MAX_MACHINES 100
#define MAX_REPEATS 100

// Structure for an operation (no pointers used - Constraint 1)
typedef struct {
    int machine;   // machine index
    int duration;  // processing time
    int start;     // start time computed
    int end;       // end time computed
} Operation;

// Static storage for all jobs and operations (Constraint 1)
int num_jobs;
int num_ops;        // number of ops per job (assumed equal to num_machines)
int num_machines;
Operation ops[MAX_JOBS][MAX_OPS];
Operation ops_backup[MAX_JOBS][MAX_OPS];

// Shared arrays
int machine_available[MAX_MACHINES];  // tracks when each machine becomes free
int job_available[MAX_JOBS];          // sequential: tracks job readiness
omp_lock_t machine_lock[MAX_MACHINES]; // one lock per machine (Constraint 3)

// ================== Input/Output ==================
void read_input(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("Error opening input file"); exit(1); }

    fscanf(fp, "%d %d", &num_jobs, &num_machines);
    num_ops = num_machines; // square assumption

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
    // Restore initial operation data and clear times
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            ops[j][i] = ops_backup[j][i];
            ops[j][i].start = 0;
            ops[j][i].end = 0;
        }
    }
}

void print_gantt_chart(FILE *fp) {
    const int block_size = 5;  // Compress time axis
    fprintf(fp, "\n# Gantt Chart (Compressed: 1 char = %d time units)\n", block_size);

    // Calculate makespan
    int makespan = 0;
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            if (ops[j][i].end > makespan)
                makespan = ops[j][i].end;
        }
    }

    int blocks = (makespan + block_size - 1) / block_size;  // total columns

    // Gantt lines by machine
    for (int m = 0; m < num_machines; m++) {
        fprintf(fp, "Machine %2d |", m);
        for (int b = 0; b < blocks; b++) {
            int t_start = b * block_size;
            int t_end = t_start + block_size;
            int printed = 0;

            for (int j = 0; j < num_jobs; j++) {
                for (int i = 0; i < num_ops; i++) {
                    if (ops[j][i].machine == m &&
                        ops[j][i].start < t_end &&
                        ops[j][i].end > t_start) {
                        fprintf(fp, "J%d", j);  // mark job
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

    // Time scale
    fprintf(fp, "\nTime       ");
    for (int b = 0; b < blocks; b++) {
        int label = b * block_size;
        if (label < 10) fprintf(fp, "  %d", label);
        else if (label < 100) fprintf(fp, " %d", label);
        else fprintf(fp, "%d", label);  // space-adjusted
    }
    // Add the exact makespan as final tick
    fprintf(fp, " %d\n", makespan);
}



// ================== Output ==================
void write_output(const char *filename, double avg_time, int repeats) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("Error opening output file"); exit(1); }

    // Compute makespan (max end time)
    int makespan = 0;
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            if (ops[j][i].end > makespan)
                makespan = ops[j][i].end;
        }
    }

    // Output makespan and operation start times
    fprintf(fp, "%d\n", makespan);
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            fprintf(fp, "%d ", ops[j][i].start);
        }
        fprintf(fp, "\n");
    }
    // Print Gantt chart
    print_gantt_chart(fp);


    // Performance section
    fprintf(fp, "\n# Performance Analysis\n");
    fprintf(fp, "Average runtime over %d repetitions: %.6f seconds\n", repeats, avg_time);
    fclose(fp);
}

// ================== Sequential Scheduling ==================
void sequential_schedule() {
    // Initialize availability
    for (int m = 0; m < num_machines; m++) machine_available[m] = 0;
    for (int j = 0; j < num_jobs; j++) job_available[j] = 0;

    // Assign start/end times respecting both machine and job constraints (Constraint 4)
    for (int j = 0; j < num_jobs; j++) {
        for (int i = 0; i < num_ops; i++) {
            int m = ops[j][i].machine;
            // Wait for both machine and job readiness
            int ready = (machine_available[m] > job_available[j]
                         ? machine_available[m] : job_available[j]);
            ops[j][i].start = ready;
            ops[j][i].end = ready + ops[j][i].duration;
            // Update when job and machine become free next
            job_available[j] = ops[j][i].end;
            machine_available[m] = ops[j][i].end;  // prevents two ops on same machine (Constraint 4)
        }
    }
}

// ================== Parallel Scheduling with OpenMP ==================
void parallel_schedule(int num_threads) {
    // Reset machine availability and initialize locks
    for (int m = 0; m < num_machines; m++) {
        machine_available[m] = 0;
        omp_init_lock(&machine_lock[m]); // one lock per machine (Constraint 3)
    }

    // Each job is processed in parallel; job order ensures op sequence
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int j = 0; j < num_jobs; j++) {
        int local_available = 0;  // when previous op in this job finished (Constraint 4)

        for (int i = 0; i < num_ops; i++) {
            int m = ops[j][i].machine;
            int d = ops[j][i].duration;

            // RACE CONDITION: without this lock multiple threads could
            // read/update machine_available[m] at the same time
            omp_set_lock(&machine_lock[m]); // lock machine (Constraint 3)

            // Compute earliest start based on both machine & job readiness
            int ready = (machine_available[m] > local_available
                         ? machine_available[m] : local_available);
            int start = ready;
            int end = start + d;

            machine_available[m] = end; // mark machine busy until 'end'
            omp_unset_lock(&machine_lock[m]); // unlock machine

            // Record schedule
            ops[j][i].start = start; // solution: when op j,i starts
            ops[j][i].end = end;
            local_available = end;   // enforce op sequence in job (Constraint 4)
        }
    }

    // Destroy locks
    for (int m = 0; m < num_machines; m++) {
        omp_destroy_lock(&machine_lock[m]);
    }
}

// ================== Timing Wrapper ==================
double measure_execution(int threads, int repeats) {
    double total_time = 0.0;
    for (int r = 0; r < repeats; r++) {
        reset_data();
        double t0 = omp_get_wtime();
        if (threads <= 1) sequential_schedule();
        else parallel_schedule(threads);
        double t1 = omp_get_wtime();
        total_time += (t1 - t0);
    }
    return total_time / repeats;  // average time
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
        fprintf(stderr, "Invalid parameters. repeats must be 1..%d\n", MAX_REPEATS);
        return 1;
    }

    double avg_time = measure_execution(threads, repeats);
    write_output(argv[2], avg_time, repeats);
    return 0;
}
