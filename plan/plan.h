#ifndef LEVEL2_PLAN_H
#define LEVEL2_PLAN_H

#include "../process.h"
#include "../task.h"
#include "../config.h"

#define ON_PLAN 0
#define SIGNALED 1


/*
 * Representation of the Plan
 */
typedef struct {
    long num_processes;
    long num_tasks;
    PlanProcess processes[MAX_NUMBER_PROCESSES] ;
    Task* tasks;
    PlanProcess* cur_process;
    Task* cur_task;
    long lateness;
    long instructions_retired;
    short state;
    long tick_counter;
} Plan;

Plan* parse_plan(char*, Plan*);
void update_retired_instructions(long instructions_retired, Plan *plan);
void update_cur_task_process(Plan*);


#endif //LEVEL2_PLAN_H
