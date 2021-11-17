#ifndef LEVEL2_PLAN_H
#define LEVEL2_PLAN_H

#include "config.h"

#define ON_PLAN 0
#define SIGNALED 1
#define PLAN_FINISHED 2


// --- main interface ---
struct PBS_Plan* parse_plan(char*,struct PBS_Plan*);
void update_retired_instructions(long instructions_retired,struct PBS_Plan *p);
void update_cur_process(struct PBS_Plan *p);

// --- helper functions ---
struct PBS_Task* find_task_with_task_id(struct PBS_Plan* , long);
void update_node_lateness(long , struct PBS_Plan* );
int number_processes_in_plan(struct PBS_Plan*);
void replace_unallocated_slot_in_plan(struct PBS_Task *replacement_task, struct PBS_Plan *p);
long calculate_length_plan(struct PBS_Plan*);


// --- for testing ---
void get_plan(struct PBS_Plan* p);
#endif //LEVEL2_PLAN_H
