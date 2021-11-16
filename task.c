//
// Created by ml on 25.03.21.
//
#include "kernel_dummies.h"

#include <stdio.h>
#include "pbs_entities.h"
#include "task.h"
#include "prediction_failure_handling.h"

/**
 * Function checks if task is late and if it has finished
 * @param instructions
 * @param task
 */
void pbs_update_retired_instructions_task(long instructions, struct PBS_Task* task) {
    task->instructions_retired_task += instructions;

    // update lateness
    if (task->instructions_retired_task > task->instructions_planned){
        task->lateness = task->instructions_retired_task - task->instructions_planned;
    }
    // check if finished
    if (task->instructions_retired_task >= task->instructions_real){
        task->lateness = task->instructions_real - task->instructions_planned;
        change_task_state(task, PLAN_TASK_FINISHED);
        return;
    }
}
EXPORT_SYMBOL(pbs_update_retired_instructions_task);

void change_task_state(struct PBS_Task *t, short state) {
    short state_before = t->state;
    t->state = state;
    if (LOG_PBS)
        printf(KERN_INFO "[change_task_state] changed Task %ld from state %d to %d\n", t->task_id, state_before, state);
}
EXPORT_SYMBOL(change_task_state);

void clear_preemption(struct PBS_Task *t){
    t->was_preempted = 0;
    t->slot_owner = t->task_id;
}

/**
 * Tracks the state changes of a task in terms of lateness
 * @param instructions_to_run Instructions run on the task
 * @param task
 * @return Task-States, as defined in Task States
 */

short does_task_turn_late(long instructions_to_run, struct PBS_Task* task){
    if (task->instructions_retired_slot >= task->instructions_planned){
        return IS_LATE;
    }
    if (task->instructions_retired_slot + instructions_to_run > task->instructions_planned)
        return TURNS_LATE;
    else
        return ON_TIME;
}
EXPORT_SYMBOL(does_task_turn_late);


/**
 * Creates a Task with the given parameter
 * @param task_id
 * @param process_id
 * @param instructions_planned
 * @param instructions_real
 * @return
 */
struct PBS_Task create_task(long task_id, long process_id, long instructions_planned, long instructions_real){
    struct PBS_Task new_task = {0};
    new_task.task_id = task_id;
    new_task.process_id = process_id;
    new_task.instructions_planned = instructions_planned;
    new_task.instructions_real = instructions_real;

    new_task.slot_owner = task_id;
    return new_task;
}
EXPORT_SYMBOL(create_task);
