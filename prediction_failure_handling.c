//
// Created by ml on 26.03.21.
//

#include "prediction_failure_handling.h"
#include "plan/plan.h"

long find_slot_to_move_to(long target_pid, Plan* p);
void move_preempted_task(Task preempted_task, long slot_to_move_to, Plan* p);

void signal_t2(Plan* p){
    p->state = SIGNALED;
    printf("signaled prediction failure for process %ld", p->cur_process->process_id);

}

/**
 * Preempts p->cur_task, moves cur_task to corresponding place in task-list, updates to new cur_task/process
 * @param p
 */
void preempt_cur_task(Plan* p){
    long next_slot_index;
    Task preempted_task = *p->cur_task;

    // move
    p->cur_task->state = TASK_PREEMPTED;
    next_slot_index = find_slot_to_move_to(p->cur_task->process_id, p);
    if (next_slot_index == -2){
        printf("found no other slot to move task too");
        //fixme: how to deal with this issue?
    }
    move_preempted_task(*p->cur_task, next_slot_index, p);

    // update
    p->cur_task->state = TASK_PREEMPTED;
    p->cur_task->slot_owner = &p->tasks[next_slot_index + 1];
    update_cur_task_process(p);
    if(LOG)
        printf("[PREEMPT_CUR_TASK] Task %ld was preempted and moved %ld slots before Task %ld\n", preempted_task.task_id,
               next_slot_index , p->tasks[next_slot_index+1].task_id);
}

/**
 * Searches the task list of p and tries to find the next slot where a task with target_pid might fit in.
 * @param target_pid
 * @param p
 * @return Returns index of next task of process relative to p->tasks pointer
 */
long find_slot_to_move_to(long target_pid, Plan* p){
    Task* next_slot = p->tasks +1;
    long pid = 0;
    long counter = 0;
    while (pid != -2){
        pid = next_slot->process_id;
        if (pid == target_pid){
            return counter;
        }
        next_slot++;
        counter++;
    }
    return -2;
}

/**
 * Moves the preempted task in front of the slot_to_move_to, moves all tasks one place forward to make space
 * !bad for performance
 * @param preempted_task
 * @param slot_to_move_to
 * @param p
 */
void move_preempted_task(Task preempted_task, long slot_to_move_to, Plan* p){
    long i = 0;
    Task next_task;
    Task* next_task_ptr;
    Task* cur_task_ptr;

    // move other tasks forward
    while (i < slot_to_move_to){
        cur_task_ptr = p->tasks+i;
        next_task_ptr = p->tasks+ i +1;
        next_task = *next_task_ptr;
        *cur_task_ptr = next_task;
        i++;
    }
    // insert preempted task before slot_to_move_to
    p->tasks[slot_to_move_to] = preempted_task;
}