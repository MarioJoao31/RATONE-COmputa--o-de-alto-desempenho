#include <stdio.h>
#include <stdlib.h>
#include <time.h>  // Include for clock()

#define NUM_JOBS 2
#define MAX_OPS  2
#define NUM_MACHINES 2

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

int best_assignment[NUM_JOBS][MAX_OPS];
int current_assignment[NUM_JOBS][MAX_OPS];
int best_makespan = 1e9;

int compute_makespan() {
    int machine_avail[NUM_MACHINES] = {0, 0};
    int job_finish[NUM_JOBS] = {0};
    for (int j = 0; j < NUM_JOBS; j++) {
        int time = 0;
        for (int o = 0; o < jobs[j].num_ops; o++) {
            int alt_index = current_assignment[j][o];
            Alternative alt = jobs[j].ops[o].alts[alt_index];
            int start = time;
            if (machine_avail[alt.machine] > start)
                start = machine_avail[alt.machine];
            int finish = start + alt.proc_time;
            machine_avail[alt.machine] = finish;
            time = finish;
        }
        job_finish[j] = time;
    }
    int mspan = 0;
    for (int j = 0; j < NUM_JOBS; j++) {
        if (job_finish[j] > mspan)
            mspan = job_finish[j];
    }
    return mspan;
}

void search_assignment(int job, int op) {
    if (job == NUM_JOBS) {
        int mspan = compute_makespan();
        if (mspan < best_makespan) {
            best_makespan = mspan;
            for (int j = 0; j < NUM_JOBS; j++)
                for (int o = 0; o < jobs[j].num_ops; o++)
                    best_assignment[j][o] = current_assignment[j][o];
        }
        return;
    }
    if (op >= jobs[job].num_ops) {
        search_assignment(job+1, 0);
        return;
    }
    for (int a = 0; a < jobs[job].ops[op].num_alts; a++) {
        current_assignment[job][op] = a;
        search_assignment(job, op+1);
    }
}

void init_instance() {
    jobs[0].num_ops = 2;
    jobs[0].ops[0].num_alts = 2;
    jobs[0].ops[0].alts = malloc(sizeof(Alternative) * 2);
    jobs[0].ops[0].alts[0].machine = 0; jobs[0].ops[0].alts[0].proc_time = 3;
    jobs[0].ops[0].alts[1].machine = 1; jobs[0].ops[0].alts[1].proc_time = 2;
    jobs[0].ops[1].num_alts = 2;
    jobs[0].ops[1].alts = malloc(sizeof(Alternative) * 2);
    jobs[0].ops[1].alts[0].machine = 0; jobs[0].ops[1].alts[0].proc_time = 2;
    jobs[0].ops[1].alts[1].machine = 1; jobs[0].ops[1].alts[1].proc_time = 4;

    jobs[1].num_ops = 2;
    jobs[1].ops[0].num_alts = 2;
    jobs[1].ops[0].alts = malloc(sizeof(Alternative) * 2);
    jobs[1].ops[0].alts[0].machine = 0; jobs[1].ops[0].alts[0].proc_time = 2;
    jobs[1].ops[0].alts[1].machine = 1; jobs[1].ops[0].alts[1].proc_time = 3;
    jobs[1].ops[1].num_alts = 2;
    jobs[1].ops[1].alts = malloc(sizeof(Alternative) * 2);
    jobs[1].ops[1].alts[0].machine = 0; jobs[1].ops[1].alts[0].proc_time = 4;
    jobs[1].ops[1].alts[1].machine = 1; jobs[1].ops[1].alts[1].proc_time = 1;
}

int main(void) {
    clock_t start_time, end_time;
    double cpu_time_used;
    
    init_instance();
    
    // Start the timer
    start_time = clock();
    
    search_assignment(0, 0);
    
    // Stop the timer
    end_time = clock();
    
    cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("Best makespan found = %d\n", best_makespan);
    printf("Time taken: %f seconds\n", cpu_time_used);
    
    for (int j = 0; j < NUM_JOBS; j++) {
        printf(" Job %d: ", j);
        for (int o = 0; o < jobs[j].num_ops; o++) {
            int a = best_assignment[j][o];
            Alternative alt = jobs[j].ops[o].alts[a];
            printf("[Op %d: M%d, t=%d] ", o, alt.machine, alt.proc_time);
        }
        printf("\n");
    }
    
    for (int j = 0; j < NUM_JOBS; j++) {
        for (int o = 0; o < jobs[j].num_ops; o++)
            free(jobs[j].ops[o].alts);
    }
    return 0;
}
