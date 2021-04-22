#ifndef LEVEL2_PLAN_H
#define LEVEL2_PLAN_H

#include "../prediction_failure_config.h"

#define ON_PLAN 0
#define SIGNALED 1
#define PLAN_FINISHED 2


/*
 * Representation of the Plan
 */
// --- main interface ---
struct PBS_Plan* parse_plan(char*,struct PBS_Plan*);
void update_retired_instructions(long instructions_retired,struct PBS_Plan *p);
void update_cur_task_process(struct PBS_Plan*);
void write_binary_to_file(struct PBS_Plan*, char*);

// --- helper functions ---
struct PBS_Task* find_task_with_task_id(struct PBS_Plan* , long);
void update_node_lateness(long , struct PBS_Plan* );
void change_plan_state(struct PBS_Plan*, short);
void show_tasks(struct PBS_Plan*);

// --- for testing ---

void generate_test_plan(struct PBS_Plan *p, struct PBS_Process* process_list, struct PBS_Task* task_list);
#endif //LEVEL2_PLAN_H
