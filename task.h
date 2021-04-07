//
// Created by ml on 23.03.21.
//

#ifndef LEVEL2_TASK_H
#define LEVEL2_TASK_H

#endif //LEVEL2_TASK_H

#define TASK_WAITING 0
#define TASK_RUNNING 1
#define TASK_PREEMPTED 2
#define TASK_SIGNALED 3
#define TASK_FINISHED 4

#define SHARES_NO_SLOT 0;

// https://stackoverflow.com/questions/3988041/how-to-define-a-typedef-struct-containing-pointers-to-itself
typedef struct Task Task;

struct Task {
    long process_id; // pid of process task belongs to
    long task_id;  // task identifier
    long instructions_planned; // instructions planned for task
    long instructions_real; // helper variable for simulating task deviations
    long instructions_retired; // instructions run on task
    long lateness; // lateness of task
    short state; // state of task, see defines for possible values
    Task* slot_owner; // It task was preempted, points to owner of slot
};

void update_retired_instructions_task(long instructions, Task* task);



