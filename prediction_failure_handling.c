//
// Created by ml on 26.03.21.
//

#include "kernel_dummies.h"
#include "pbs_entities.h"
#include "prediction_failure_handling.h"
#include "plan/plan.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>


static int cur_sig_buffer_position;
static PredictionFailureSignal lastSignals [SIZE_SIG_BUFFER]; // ring buffer to track latest signals

void reschedule(struct PBS_Plan* p, short);

// --- debug ---
void get_all_ids_from_plan(long[400], struct PBS_Plan*);
short same_ids(long[400], long[400]);
short order_is_kept(struct PBS_Plan*);

void handle_no_preemption_slot_found(struct PBS_Plan *p);

void signal_t2(struct PBS_Plan* p){
    PredictionFailureSignal sig;
    change_plan_state(p, SIGNALED);
    printf(KERN_ALERT "[PBS_signal_t2]%ld: signaled prediction failure for process %ld", p->tick_counter, p->cur_process->process_id);
    p->stress = STRESS_PER_SIGNAL;

    sig.cur_task_id = p->cur_task->task_id,
    sig.cur_process_id = p->cur_process->process_id;
    sig.tick = p->tick_counter;
    sig.type_signal = STRETCH_SIGNAL;

    add_signal(sig);

    reschedule(p, STRETCH_SIGNAL);
}

void signal_tm2(struct PBS_Plan* p){
    PredictionFailureSignal  sig;
    change_plan_state(p, SIGNALED);
    printf(KERN_ALERT"[PBS_signal_tm2]%ld: signaled prediction failure for task %ld\n",p->tick_counter, p->cur_task->task_id);
    p->stress = STRESS_PER_SIGNAL;

    sig.cur_task_id = p->cur_task->task_id,
    sig.cur_process_id = p->cur_process->process_id;
    sig.tick = p->tick_counter;
    sig.type_signal = SHRINK_SIGNAL;

    add_signal(sig);

    reschedule(p, SHRINK_SIGNAL);
}
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
   long next_free_slot_index;
   int stack_size;
   long insertion_slot;
   struct PBS_Task preempted_tasks[T2_MAX_PREEMPTIONS+1] = {0};

    insertion_slot= find_slot_to_move_to(p->cur_task->process_id, p);
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

void handle_no_preemption_slot_found(struct PBS_Plan *p) {//fixme: how to deal with this issue? State: Just sign PBS_Taskfinished
    printf(KERN_ALERT"[PBS_handle_no_preemption_slot_found]%ld: Found no other slot to move task to \n", p->tick_counter);
    long instructions_missing = p->cur_task->instructions_real - p->cur_task->instructions_retired_task;
    update_retired_instructions(instructions_missing, p);
    printf(KERN_EMERG "[PBS_handle_no_preemption_slot_found]%ld: no preemption slot found\n", p->tick_counter);
    p->state = PLAN_FINISHED;
}

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
            printf(KERN_DEBUG "[PBS_find_slot_to_move_to]%ld, found slot %ld for (%ld, %ld)\n",p->tick_counter, counter - 1, p->cur_task->task_id, p->cur_task->process_id);
            assert(counter >= 1 );
            return counter - 1;
        }
        next_slot++;
        counter++;
    }
    return -2;
}

/**
 * Moves the preempted task in front of the slot_to_move_to, updates slot owner
 * @param insertion_slot : Slot where stack should be inserted
 * @param stack_size  : Size of the stack
 * @param preempted_tasks : Ptr to preempted tasks
 * @param p
 */
void move_preempted_tasks(long insertion_slot, int stack_size, struct PBS_Task* preempted_tasks,struct PBS_Plan *p) {
    long new_slot_owner = p->tasks[insertion_slot+1].task_id;
    long cur_slot;

    // set new slot owners
    for (int i = 0; i < stack_size; i++){
        preempted_tasks[i].slot_owner = new_slot_owner;
    }

    cur_slot = insertion_slot-stack_size+1;
    struct PBS_Task* cur_task = &p->tasks[cur_slot];
    struct PBS_Task task_before = *cur_task;

    for (int i = 0; i < stack_size; i++){
        *(cur_task + i) = preempted_tasks[i];

        printf(KERN_DEBUG "[PBS_move_preempted_tasks]%ld: Moved (%ld,%ld) in place of (%ld,%ld)\n", p->tick_counter,
               cur_task->task_id, cur_task->process_id, task_before.task_id, task_before.process_id);
    }

}
/**
 * Checks what tasks have to be moved by the preemption, are stored in tasks-to_move
 * @param p current plan
 * @param tasks_to_move adresses of tasks that have to be moved
 * @returns the number of tasks that have to be moved
 */
int get_stack_size_preempted_tasks(struct PBS_Task *tasks_to_move, struct PBS_Plan* p){
    int index = 0;
    short add_next = 1;
    struct PBS_Task* task_to_check = p->cur_task;

    //
    while(task_to_check->slot_owner != SHARES_NO_SLOT && task_to_check->process_id == p->cur_task->process_id){
        tasks_to_move[index] = *(task_to_check);
        index++;
        task_to_check++;
    }

    // add slot that was previous slot owner
    tasks_to_move[index] = *task_to_check;

    for(int i = 0; i<index; i++){
        tasks_to_move[i].state = PLAN_TASK_PREEMPTED;
    }

    return index + 1;
}

/**
 *  Moves tasks for to enable preemption
 * @param insertion_slot
 * @param stack_size
 * @param p
 */
void move_other_tasks_forward(long insertion_slot, long stack_size, struct PBS_Plan *p) {
    // move other tasks forwards
    for (int i = 0; i < insertion_slot; i++) {
        p->tasks[i] = p->tasks[i + stack_size];
    }
}
/**
 * Simulates a rescheduling from the scheduling component
 * @param p
 */
void reschedule(struct PBS_Plan* p, short signal){
    // find out for what tasks rescheduling has to occur
    long instructions_rescheduling = RESCHEDULE_TIME * INS_PER_TICK;
    long new_length;
    long stretch_factor;
    long task_counter = 0;
    long target_pid = p->cur_process->process_id;

    struct PBS_Task* cur_task = p->cur_task;
    while (instructions_rescheduling > 0){
         instructions_rescheduling -= cur_task->instructions_real;
         cur_task++;
     }

    // apply stretch
    if(signal == STRETCH_SIGNAL)
        stretch_factor = STRETCH_CONSTANT;
    else
        stretch_factor = SHRINK_CONSTANT;

    while (cur_task->process_id != -2){
        if (cur_task->process_id == target_pid) {
            new_length = (cur_task->instructions_planned * stretch_factor) / 100;
            cur_task->instructions_planned = new_length;
            task_counter++;
        }
        cur_task++;
    }
    printf(KERN_DEBUG "[PBS_reschedule]%ld: Received %d signal and stretched/shrunk %ld tasks\n", p->tick_counter, signal, task_counter);
    }

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
    for (int i = 0; i < 400; i++){
        if (first[i] == LONG_MAX)
            break;
        for(int j = 0; j < 400; j++){
            if (first[i] == second[j] && first[i] >= 0){
                first[i] = first[i] * -1;
                second[j] = second[j] * -1;
                break;
            }
        }
    }
    for (int i = 0; i < 400; i++){
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
            printf(KERN_INFO "[PBS_order_is_kept]%ld: %ld is before %ld\n", p->tick_counter, p->cur_task->task_id, last_id[cur_task->process_id]);
            return 0;
        } else {
            last_id[cur_task->process_id] = cur_task->task_id;
        }
    }
    return 1;
}

/**
 *
 * @param sig
 */
void add_signal(PredictionFailureSignal sig){
    cur_sig_buffer_position++;
    if (cur_sig_buffer_position == 100)
        cur_sig_buffer_position = 0;
    lastSignals[cur_sig_buffer_position] = sig;
}
/**
 * Returns latest signals in lastSignals
 * @param pick_signal Index of latest signal, 0 -> latest signal, 1 2nd latest signal,...
 * @return
 */
PredictionFailureSignal* get_signal(int pick_signal){
    assert(pick_signal >= 0 && pick_signal < SIZE_SIG_BUFFER);
    int target_index =  cur_sig_buffer_position - pick_signal;

    // no warp around happening
    if (target_index >= 0)
        return &lastSignals[target_index];
    // wrap around happens
    target_index = SIZE_SIG_BUFFER + target_index;
    return &lastSignals[target_index];
}
