//
// Created by ml on 25.03.21.
//

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
        task->state = TASK_FINISHED;
    }
}


