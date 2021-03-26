//
// Created by ml on 23.03.21.
//

#ifndef LEVEL2_TASK_H
#define LEVEL2_TASK_H

#endif //LEVEL2_TASK_H

typedef struct {
    long process_id;
    long task_id;
    long instructions_planned;
    long instructions_real;
    long instructions_retired;
    long lateness;
} Task;

void update_retired_instructions_task(long instructions, Task* task);



