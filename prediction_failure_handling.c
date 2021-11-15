#include "kernel_dummies.h"
#include "pbs_entities.h"
#include "prediction_failure_handling.h"
#include "plan.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>


int get_stack_size_preempted_tasks(struct PBS_Task *tasks_to_move, struct PBS_Plan* p);
// --- debug ---
void get_all_ids_from_plan(long[400], struct PBS_Plan*);
short same_ids(long[400], long[400]);
short order_is_kept(struct PBS_Plan*);

void handle_no_preemption_slot_found(struct PBS_Plan *p);



/**
 * Implements a task preemption for the current task, does the following actions:
 * - checks if more then one PBS_Task is assigned to the current slot
 * - looks for the next slot fitting the preempted task
 * - moves preempted task and associated tasks in the task list backwards, moves other tasks forward
 * - changes states to reflect task preemption
 * - changes current task/process
 * @param p
 */
void preempt_cur_task(struct PBS_Plan* p){
   int stack_size;
   long insertion_slot;
   struct PBS_Task preempted_tasks[T2_MAX_PREEMPTIONS+1] = {{0}};
    p->cur_task->was_preempted++;
    insertion_slot = find_slot_to_move_to(p->cur_task->process_id, p);
    stack_size = get_stack_size_preempted_tasks(preempted_tasks, p);

    if (insertion_slot == -2){
        handle_no_preemption_slot_found(p);
        return;
    }
    move_other_tasks_forward(insertion_slot, stack_size, p);
    move_preempted_tasks(insertion_slot, stack_size, preempted_tasks, p);

    // update

    update_cur_task_process(p);
    if(LOG_PBS)
        printf(KERN_WARNING "[PBS_preempt_cur_task]%ld: %d Tasks where preempted and moved into slot %ld before PBS_Task%ld\n", p->tick_counter, stack_size,
               insertion_slot , p->tasks[insertion_slot+1].task_id);
}

EXPORT_SYMBOL(preempt_cur_task);

void handle_no_preemption_slot_found(struct PBS_Plan *p) {//fixme: how to deal with this issue? State: Just sign PBS_Taskfinished
    long instructions_missing = p->cur_task->instructions_real - p->cur_task->instructions_retired_task;
    printf(KERN_ALERT"[PBS_handle_no_preemption_slot_found]%ld: Found no other slot to move task to \n", p->tick_counter);
    update_retired_instructions(instructions_missing, p);
    printf(KERN_EMERG "[PBS_handle_no_preemption_slot_found]%ld: no preemption slot found\n", p->tick_counter);
    p->state = PLAN_FINISHED;
}

EXPORT_SYMBOL(handle_no_preemption_slot_found);

/**
 * Searches the task list of p and tries to find the next slot where a task with target_pid might fit in.
 * @param target_pid
 * @param p
 * @return returns index relative to p->tasks where preempted_task can be moved to; returns -2 if no slot can be found
 */
long find_slot_to_move_to(long target_pid, struct PBS_Plan* p){
    struct PBS_Task* next_slot = p->tasks;
    short state;
    long pid = 0;
    long counter = 0;

    // 0-0-1-0 => go to 1, go behind current patch of tasks with target_pid
    while(next_slot->process_id == target_pid){
        next_slot++;
        counter++;
    }

    while (pid != -2){
        pid = next_slot->process_id;
        state = next_slot->state;
        if (pid == target_pid){
            if (LOG_PBS)
                printf(KERN_DEBUG "[PBS_find_slot_to_move_to]%ld, found slot %ld for (%ld, %ld)\n",p->tick_counter, counter - 1, p->cur_task->task_id, p->cur_task->process_id);
            assert(counter >= 1 );
            return counter - 1;
        }
        next_slot++;
        counter++;
    }
    return -2;
}

EXPORT_SYMBOL(find_slot_to_move_to);

/**
 * Moves the preempted task in front of the slot_to_move_to, updates slot owner
 * @param insertion_slot : Slot where stack should be inserted
 * @param stack_size  : Size of the stack
 * @param preempted_tasks : Ptr to preempted tasks
 * @param p
 */
void move_preempted_tasks(long insertion_slot, int stack_size, struct PBS_Task* preempted_tasks,struct PBS_Plan *p) {
    long cur_slot;
    int i;
    struct PBS_Task* cur_task, task_before;
    long new_slot_owner = p->tasks[insertion_slot+1].task_id;

    // set new slot owners
    for ( i = 0; i < stack_size; i++){
        preempted_tasks[i].slot_owner = new_slot_owner;
    }

    cur_slot = insertion_slot-stack_size+1;
    cur_task = &p->tasks[cur_slot];
    task_before = *cur_task;

    for ( i = 0; i < stack_size; i++){
        *(cur_task + i) = preempted_tasks[i];
        if (LOG_PBS)
            printf(KERN_DEBUG "[PBS_move_preempted_tasks]%ld: Moved (%ld,%ld) in place of (%ld,%ld)\n", p->tick_counter,
               cur_task->task_id, cur_task->process_id, task_before.task_id, task_before.process_id);
    }

}

EXPORT_SYMBOL(move_preempted_tasks);

/**
 * Checks what tasks have to be moved by the preemption, are stored in tasks-to_move
 * @param p current plan
 * @param tasks_to_move adresses of tasks that have to be moved
 * @returns the number of tasks that have to be moved
 */
int get_stack_size_preempted_tasks(struct PBS_Task *tasks_to_move, struct PBS_Plan* p){
    int i;
    int index = 0;
    struct PBS_Task* task_to_check = p->cur_task;

    //
    while(task_to_check->slot_owner != task_to_check->task_id && task_to_check->process_id == p->cur_task->process_id){
        tasks_to_move[index] = *(task_to_check);
        index++;
        task_to_check++;
    }

    // add slot that was previous slot owner
    tasks_to_move[index] = *task_to_check;

    for( i = 0; i<index; i++){
        tasks_to_move[i].state = PLAN_TASK_PREEMPTED;
    }

    return index + 1;
}

EXPORT_SYMBOL(get_stack_size_preempted_tasks);

/**
 * Moves all tasks forward to make room for the preempted task
 * @param insertion_slot
 * @param stack_size
 * @param p
 */
void move_other_tasks_forward(long insertion_slot, long stack_size, struct PBS_Plan *p) {
    // move other tasks forwards
    int i;
    for ( i = 0; i < insertion_slot; i++) {
        p->tasks[i] = p->tasks[i + stack_size];
    }
}

EXPORT_SYMBOL(move_other_tasks_forward);




// --- debug ---
void get_all_ids_from_plan(long list_ids[400], struct PBS_Plan* p){
    struct PBS_Task* cur_task = p->tasks;
    int counter = 0;
    while (cur_task->task_id != -2){
        list_ids[counter] = cur_task->task_id;
        cur_task++;
        counter++;
    }
    list_ids[counter] = LONG_MAX;
}


short same_ids(long first[400], long second[400]){
    int i, j;
    for ( i = 0; i < 400; i++){
        if (first[i] == LONG_MAX)
            break;
        for( j = 0; j < 400; j++){
            if (first[i] == second[j] && first[i] >= 0){
                first[i] = first[i] * -1;
                second[j] = second[j] * -1;
                break;
            }
        }
    }
    for ( i = 0; i < 400; i++){
        if (first[i] ==  LONG_MAX)
            break;
        if (first[i] > 0 || second[i] > 0){
            return 0;
        }
    }
    return 1;
}
// checks if the task-ids for each process are increasing
short order_is_kept(struct PBS_Plan* p){
    long last_id[MAX_NUMBER_PROCESSES] = {0};
    struct PBS_Task* cur_task = p->tasks;
    for (;cur_task->task_id != -2; cur_task++){
        if(cur_task->task_id == -1)
            continue;
        if(cur_task->task_id < last_id[cur_task->process_id]) {
            if (LOG_PBS)
                printf(KERN_INFO "[PBS_order_is_kept]%ld: %ld is before %ld\n", p->tick_counter, p->cur_task->task_id, last_id[cur_task->process_id]);
            return 0;
        } else {
            last_id[cur_task->process_id] = cur_task->task_id;
        }
    }
    return 1;
}
