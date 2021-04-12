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
#define TASK_ABORTED 5

#define SHARES_NO_SLOT 0


typedef struct {
    long task_id;  // task identifier
    long process_id; // pid of process task belongs to
    long instructions_planned; // instructions planned for task
    long instructions_real; // helper variable for simulating task deviations
    long instructions_retired_task; // instructions run on task
    long instructions_retired_slot; // instructions run on original slot of task
    long lateness; // lateness of task
    short state; // state of task, see defines for possible values
    long slot_owner; // ID of task in which current task runs; should be pointer for efficiency, but none trivial with task preemption and moving of tasks in memory
} Task;

void update_retired_instructions_task(long instructions, Task *task);
void change_task_state(Task *t, short state);


