//
// Created by ml on 25.03.21.
//

#include <stdio.h>
#include "pbs_entities.h"
#include "task.h"
#include "prediction_failure_handling.h"

void update_retired_instructions_task(long instructions, struct PBS_Task*task) {
    task->instructions_retired_task += instructions;

    // update lateness
    if (task->instructions_retired_task > task->instructions_planned){
        task->lateness = task->instructions_retired_task - task->instructions_planned;
    }
    // check if finished
    if (task->instructions_retired_task >= task->instructions_real){
        task->lateness = task->instructions_real - task->instructions_planned;
        change_task_state(task, PLAN_TASK_FINISHED);
    }
}

void change_task_state(struct PBS_Task *t, short state) {
    short state_before = t->state;
    t->state = state;
    printf("[CHANGE_TASK_STATE] changed task %ld from %d to %d\n", t->task_id, state_before, state);
}

