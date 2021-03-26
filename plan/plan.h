#ifndef LEVEL2_PLAN_H
#define LEVEL2_PLAN_H

#include "../process.h"
#include "../task.h"

/*
 * Representation of the Plan
 */
typedef struct {
    long num_processes;
    long num_tasks;
    PlanProcess* processes;
    Task* tasks;
    PlanProcess* cur_process;
    Task* cur_task;
    long lateness;
    long instructions_retired;
} Plan;

Plan* parse_plan(char*, Plan*);
void update_retired_instructions(long, Plan*);


#endif //LEVEL2_PLAN_H
