//
// Created by ml on 25.03.21.
//

#include <stdio.h>
#include "task.h"

void update_retired_instructions_task(long instructions, Task* task){
    task->instructions_retired_task += instructions;

    // update lateness
    if (task->instructions_retired_task > task->instructions_planned){
        task->lateness = task->instructions_retired_task - task->instructions_planned;
    }
    // check if finished
    if (task->instructions_retired_task >= task->instructions_real){
        task->lateness = task->instructions_real - task->instructions_planned;
        change_task_state(task, TASK_FINISHED);
    }
}

void change_task_state(Task* t, short state){
    short state_before;
    t->state = state;
    printf("[CHANGE_TASK_STATE] changed task %ld from %d to %d\n", t->task_id, state_before, state);
}

