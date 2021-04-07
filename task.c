//
// Created by ml on 25.03.21.
//

#include "task.h"

void update_retired_instructions_task(long instructions, Task* task){
    task->instructions_retired += instructions;
    if (task->instructions_retired > task->instructions_planned){
        task->lateness = task->instructions_retired - task->instructions_planned;
    }
    if (task->instructions_retired >= task->instructions_real){
        task->state = TASK_FINISHED;
    }
}


