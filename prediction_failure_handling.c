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
short no_id_is_duplicated(struct PBS_Plan* p);


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
    assert(p->cur_task->task_id != -1);
   int stack_size;
   long insertion_slot;
   struct PBS_Task preempted_tasks[T2_MAX_PREEMPTIONS+1] = {{0}};
   struct PBS_Task preempted_task = *p->cur_task;

    p->cur_task->was_preempted++;
    insertion_slot = find_slot_to_move_to(p->cur_task->process_id, p); // returns slot index before next task of same PID
    assert(insertion_slot > p->index_cur_task);
    stack_size = get_stack_size_preempted_tasks(preempted_tasks, p);

    if (insertion_slot == -2){
        handle_no_preemption_slot_found(p);
        return;
    }

    move_tasks(insertion_slot, stack_size, p);
    assert(no_id_is_duplicated(p));
    if(LOG_PBS)
        printf(KERN_INFO "[PBS_preempt_cur_task]%ld: Task %ld was preempted and moved into slot %ld before Task %ld\n", p->tick_counter, preempted_task.task_id,
               insertion_slot , p->tasks[insertion_slot+1].task_id);
}

EXPORT_SYMBOL(preempt_cur_task);

void handle_no_preemption_slot_found(struct PBS_Plan *p) {//fixme: how to deal with this issue? State: Just sign PBS_Taskfinished
    long instructions_missing = p->cur_task->instructions_real - p->cur_task->instructions_retired_task;
    printf(KERN_ALERT"[handle_no_preemption_slot_found]%ld: Found no other slot to move task to \n", p->tick_counter);
    update_retired_instructions(instructions_missing, p);
    printf(KERN_EMERG "[handle_no_preemption_slot_found]%ld: no preemption slot found\n", p->tick_counter);
    p->state = PLAN_FINISHED;
}

EXPORT_SYMBOL(handle_no_preemption_slot_found);

/**
 * Searches the task list to find the next slot where a task with target_pid might fit in.
 * @param target_pid
 * @param p
 * @return returns index of next slot BEFORE other process with same ID; returns -2 if no slot can be found
 */
long find_slot_to_move_to(long target_pid, struct PBS_Plan* p){
    struct PBS_Task* next_slot = p->cur_task;
    short state;
    long pid = 0;
    long insertion_slot = p->index_cur_task;

    // 0-0-1-0 => go to 1, go behind current patch of tasks with target_pid
    while(next_slot->process_id == target_pid){
        next_slot++;
        insertion_slot++;
    }

    while (pid != -2){
        pid = next_slot->process_id;
        state = next_slot->state;
        if (pid == target_pid){
            if (LOG_PBS)
                printf(KERN_DEBUG "[find_slot_to_move_to]%ld: found slot %ld with task (tid: %ld, pid: %ld)"
                                  " for task(tid: %ld, pid: %ld)\n", p->tick_counter, insertion_slot, p->tasks[insertion_slot].task_id,
                       p->tasks[insertion_slot].process_id, p->cur_task->task_id, p->cur_task->process_id);
            assert(insertion_slot >= 1 );
            return insertion_slot - 1;
        }
        next_slot++;
        insertion_slot++;
    }
    return -2;
}

EXPORT_SYMBOL(find_slot_to_move_to);

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
 * Move preempted tasks to their corresponding insertion slot and shifts all other tasks forward so no gaps arise
 * @param insertion_slot
 * @param stack_size
 * @param p
 */
void move_tasks(long insertion_slot, long stack_size, struct PBS_Plan *p) {
    // move other tasks forwards
    long i;
    long insertion_slot_per_task;
    struct PBS_Task preempted_tasks [stack_size];
    long slot_id_for_preempted_tasks = p->tasks[insertion_slot + 1].task_id;


    print_plan_state(p, p->index_cur_task, insertion_slot+1);
    // save preempted tasks
    for (i = 0; i < stack_size; i++){
        preempted_tasks[i] = *(p->cur_task + i);
    }

    // update slot information for preempted tasks
    for(i = 0; i < stack_size; i++){
        preempted_tasks[i].slot_owner = slot_id_for_preempted_tasks;
    }

    // move other tasks forward
    for (i = p->index_cur_task; i < insertion_slot; i++) {
        insertion_slot_per_task = i + stack_size; // move to first task that is not of same pid
        p->tasks[i] = p->tasks[insertion_slot_per_task];
    }
    // write preempted tasks to insertion slot
    for(i = 1; i <= stack_size; i++){
        insertion_slot_per_task = insertion_slot - stack_size; // where area starts to insert into
        insertion_slot_per_task += i; //which task of stack
        p->tasks[insertion_slot_per_task] = preempted_tasks[i-1];
    }

    // update
    update_cur_process(p);
    print_plan_state(p, p->index_cur_task, insertion_slot);

}
EXPORT_SYMBOL(move_tasks);




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

/**
 * Check if an tid is duplicated, should never be the case (except for idle slots with tid -1)
 * @param p
 * @return 1 if no duplicates, else 0
 */
short no_id_is_duplicated(struct PBS_Plan* p){
    long i = 0;
    struct PBS_Task *task_ptr = &p->tasks[0];
    short found[MAX_NUMBER_TASKS_IN_PLAN] = {0};

    while(task_ptr->task_id != -2 || i >= MAX_NUMBER_TASKS_IN_PLAN ){
        if (task_ptr->task_id == -1) {
            task_ptr++; i++;
            continue;
        }
        if (found[task_ptr->task_id] != 0) {
            printf(KERN_ERR "[no_id_is_duplicated]%ld: tid %ld duplicated\n", p->tick_counter, task_ptr->task_id);
            return 0;
        }
        else found[task_ptr->task_id]++;
        task_ptr++; i++;
    }

    return 1;
}