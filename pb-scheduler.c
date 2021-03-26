//
// Created by ml on 26.03.21.
//
#include <stdio.h>
#include "pb-scheduler.h"
#include "pmu_interface.h"
#include "plan/plan.h"

void update(long, Plan*);
void check_threhsolds(Plan*,PlanProcess*, Task*);

void schedule(Plan *plan) {
    //todo: Just for checking:
    plan->cur_task = &plan->tasks[0];
    plan->cur_process = &plan->processes[0];
    long retired_instructions = get_retired_instructions();
    update(retired_instructions, plan);
}

void update(long retired_instructions, Plan* plan){
    update_retired_instructions(retired_instructions, plan);
    printf("cur_task: %ld: instructions_retired: %ld \n", plan->cur_task->task_id, plan->cur_task->instructions_retired);
}