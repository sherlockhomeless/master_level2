//
// Created by ml on 26.03.21.
//

#include <stdio.h>
#include "plan/plan.h"

#ifndef LEVEL2_PREDICTION_FAILURE_HANDLING_H
#define LEVEL2_PREDICTION_FAILURE_HANDLING_H

#define STRETCH_SIGNAL 0
#define SHRINK_SIGNAL 1

#endif //LEVEL2_PREDICTION_FAILURE_HANDLING_H

// --- CONFIG VARIABLES ---

void signal_t2(Plan*);
void signal_tm2(Plan*);
void preempt_cur_task(Plan*);

// --- for testing ---
int get_stack_size_preempted_tasks(Task *tasks_to_move, Plan *p);
long find_slot_to_move_to(long target_pid, Plan* p);
void move_preempted_tasks(long insertion_slot, int stack_size, Task *preempted_tasks, Plan *p);
void move_other_tasks_forward(long insertion_slot, long stack_size, Plan* p);