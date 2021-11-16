#include <stdio.h>
#include <assert.h>

#include "kernel_dummies.h"
#include "pbs_entities.h"
#include "plan.h"
#include "task.h"
#include "process.h"
#include "config.h"


void update_free_space_usage(long, struct PBS_Plan *);

/**
 * Updates data structures to reflect state of plan if instructions_retired are run
 * @param instructions_retired PMU-Counter since it was last read
 * @param p
 */
void update_retired_instructions(long instructions_retired, struct PBS_Plan* p){
    struct PBS_Task * slot_owner;
    short task_state;

    // ???
    if (instructions_retired < 0){
        update_free_space_usage(instructions_retired, p);
        return;
    }

    task_state = does_task_turn_late(instructions_retired, p->cur_task);

    // --- update on tasks ---
    pbs_update_retired_instructions_task(instructions_retired, p->cur_task);

    // --- update retired instructions on slot ---
    if (p->cur_task->slot_owner == p->cur_task->task_id){ // no preemption
        p->cur_task->instructions_retired_slot += instructions_retired;
    } else { // preemption
        slot_owner = find_task_with_task_id(p, p->cur_task->slot_owner);
        slot_owner->instructions_retired_slot += instructions_retired;
    }

    // --- update  instructions on process/p ---
    update_retired_instructions_process(instructions_retired, p->cur_process);
    p->instructions_retired += instructions_retired;
}
EXPORT_SYMBOL(update_retired_instructions);

/**
 * Updates cur_process to reflect current state
 * @param p
 */
void update_cur_process(struct PBS_Plan *p) {
    change_task_state(p->cur_task, PLAN_TASK_RUNNING);
    if (p->cur_task->task_id != -1)
        p->cur_process = &p->processes[p->cur_task->process_id];
    else
        p->cur_process = &p->processes[MAX_NUMBER_PROCESSES -1];
    if(LOG_PBS)
        printf(KERN_INFO "[PBS_update_cur_task_process]%ld: (%ld,%ld) is new cur_task\n", p->tick_counter,
           p->cur_task->task_id, p->cur_task->process_id);
}

/**
 * Helps find a task in p->tasks with given task_id
 * @param p
 * @param task_id
 * @return
 */
struct PBS_Task* find_task_with_task_id(struct PBS_Plan* p, long task_id){
    struct PBS_Task* cur_task = &p->tasks[0];
    while (cur_task->task_id != -2){
        if (cur_task->task_id == task_id){
            assert(cur_task != NULL);
            return cur_task;
        }
        cur_task++;
    }
    return 0;
}
/**
 * Incorporates free space into lateness calculation for node lateness
 * @param length_free_space
 * @param p
 */
void update_free_space_usage(long length_free_space, struct PBS_Plan* p){
    p->lateness += length_free_space;
    if(LOG_PBS){
        printf(KERN_INFO "[PBS_update_free_space_usage]%ld, Node-lateness changed from %ld to %ld",p->tick_counter, p->lateness + length_free_space, p->lateness );
    }
}

void update_node_lateness(long instructions, struct PBS_Plan* p){
    p->lateness += instructions;
}

void get_plan(struct PBS_Plan* p){
    int i;
    p->cur_task = &p->tasks[0];
    p->finished_tasks = &p->tasks[0];
    p->cur_process = &p->processes[p->cur_task->process_id];
    for (i = 0; i < MAX_NUMBER_TASKS_IN_PLAN; i++) {
        p->tasks[i] = create_task(i % 3, i, i, i);
    }
    p->tasks[MAX_NUMBER_TASKS_IN_PLAN-1].task_id = -2;
}


// here because of p->num_processes might not be initialized warning
int number_processes_in_plan(struct PBS_Plan* p){
    int num = 0;
    struct PBS_Process* cur_process = p->processes;
    while (cur_process->process_id != -2){
        cur_process++;
        num++;
    }
    return num;
}
EXPORT_SYMBOL(number_processes_in_plan);

/**
 * Replaces the current task at p->tasks[0] (supposed to be an unallocated time slot) with the task pointed to by replacement_task
 * @param insertion_index
 * @param replacement_task
 * @param p
 */
void replace_unallocated_slot_in_plan(struct PBS_Task* replacement_task, struct PBS_Plan *p) {
    int i;
    struct PBS_Task tmp;
    struct PBS_Task* cur_task = replacement_task;

    // set current task to replacement task
    p->tasks[0] = *replacement_task;

    // move all tasks behind replacement_task one forward
    while (cur_task->task_id != -2){
        *cur_task = *(cur_task+1);
        cur_task++;
    }
}
EXPORT_SYMBOL(replace_unallocated_slot_in_plan);

