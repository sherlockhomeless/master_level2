//
// Created by ml on 26.03.21.
//

#include "prediction_failure_handling.h"
#include "plan/plan.h"

int find_tasks_to_move(Task tasks_to_move[6], Plan *p);
long find_slot_to_move_to(long target_pid, Plan* p);
void move_preempted_tasks(long slot_to_move_to, int num_task_to_move, Task tasks_to_move[T2_MAX_PREEMPTIONS + 1], Plan *p);

void signal_t2(Plan* p){
    change_plan_state(p, SIGNALED);
    printf("[SIGNAL_T3] signaled prediction failure for process %ld", p->cur_process->process_id);
}

/**
 * Implements a task preemption for the current task, does the following actions:
 * - checks if more then one Task is assigned to the current slot
 * - looks for the next slot fitting the preempted task
 * - moves preempted task and associated tasks in the task list backwards, moves other tasks forward
 * - changes states to reflect task preemption
 * - changes current task/process
 * @param p
 */
void preempt_cur_task(Plan* p){
    int index = 0;
    int num_tasks_2_move;
    short shares_slot = p->cur_task->slot_owner;
    long next_slot_index;
    Task preempted_task = *p->cur_task;
    Task tasks_2_move[T2_MAX_PREEMPTIONS+1];


    num_tasks_2_move = find_tasks_to_move(tasks_2_move, p);
    // change task state
    while(tasks_2_move[index].slot_owner != -1){
        change_task_state(&tasks_2_move[index], TASK_PREEMPTED);
        index++;
    }

    next_slot_index = find_slot_to_move_to(p->cur_task->process_id, p);

    if (next_slot_index == -2){
        //fixme: how to deal with this issue? State: Just sign Task finished
        printf("[PREEMPT_CUR_TASK]%ld: Found no other slot to move task to \n", p->tick_counter);
        long instructions_missing = p->cur_task->instructions_real - p->cur_task->instructions_retired_task;
        update_retired_instructions(instructions_missing, p);
        return;

    }
    move_preempted_tasks(next_slot_index, num_tasks_2_move, tasks_2_move, p);

    // update
    p->cur_task->slot_owner = p->tasks[next_slot_index+1].task_id;
    update_cur_task_process(p);
    if(LOG)
        printf("[PREEMPT_CUR_TASK]%ld: Task %ld was preempted and moved %ld slots before Task %ld\n", p->tick_counter, preempted_task.task_id,
               next_slot_index , p->tasks[next_slot_index+1].task_id);
}

/**
 * Searches the task list of p and tries to find the next slot where a task with target_pid might fit in.
 * @param target_pid
 * @param p
 * @return Returns index of next task of process relative to p->tasks pointer
 */
long find_slot_to_move_to(long target_pid, Plan* p){
    Task* next_slot = p->tasks;

    CONTINUE_HERE => Problem, der nächste SLot wird nicht rifhtig gefunden und weil eine zusammenhängenge Scheiße
    short state;
    long pid = 0;
    long counter = 0;
    while (pid != -2){
        pid = next_slot->process_id;
        state = next_slot->state;
        if (pid == target_pid && state == TASK_WAITING){
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
void move_preempted_tasks(long slot_to_move_to, int num_task_to_move, Task tasks_to_move[T2_MAX_PREEMPTIONS + 1], Plan *p) {
    Task next_task;
    Task* next_task_ptr;
    Task* cur_task_ptr;
    Task debug_task;
    long target_slot;

    show_tasks(p);
    // move other tasks forwards
    for (int i = 0; i < slot_to_move_to; i++){
        p->tasks[i] = p->tasks[i+num_task_to_move];
    }

    // insert preempted tasks before slot_to_move_to
    for (int i = 0; i < num_task_to_move; i++){
        target_slot = slot_to_move_to - num_task_to_move + 1;
        p->tasks[target_slot] = tasks_to_move[i];
        debug_task = p->tasks[target_slot];

    }
    show_tasks(p);
}
/**
 * Checks what tasks have to be moved by the preemption
 * @param p current plan
 * @param tasks_to_move adresses of tasks that have to be moved
 * @returns the number of tasks that have to be moved
 */
int find_tasks_to_move(Task tasks_to_move[T2_MAX_PREEMPTIONS+1], Plan* p){
    int index = 0;
    short add_next = 1;
    Task* task_to_check = p->cur_task;

    do {
        tasks_to_move[index] = *task_to_check;
        if (task_to_check->slot_owner == 0){
            add_next = 0;
        }
        task_to_check++;
        index++;
    } while (task_to_check->slot_owner != 0 || add_next);

    tasks_to_move[index].slot_owner = -1;
    return index;
}