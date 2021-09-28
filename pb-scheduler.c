//
// Created by ml on 26.03.21.
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pbs_entities.h"
#include "kernel_dummies.h"
#include "pb-scheduler.h"
#include "pmu_interface.h"
#include "plan.h"
#include "process.h"
#include "task.h"
#include "threshold_checking.h"
#include "prediction_failure_handling.h"
#include "config.h"

void schedule_task_finished(struct PBS_Plan*);
void schedule_timer_tick(struct PBS_Plan*);
void switch_task(struct PBS_Plan*);
void handle_free_slot(struct PBS_Plan*);
void handle_unallocated_slot(struct PBS_Plan* p);
struct PBS_Process* find_latest_process(struct PBS_Plan* p);
void idle(struct PBS_Plan*);

void
find_next_task_for_all_processes(const struct PBS_Plan *p, int i, long found_all_processes, struct PBS_Task *cur_task,
                                 struct PBS_Task *next_tasks_each_process);

struct PBS_Plan pbs_plan = {0};

struct PBS_Plan* pbs_plan_ptr = &pbs_plan;

static int times_address_read = 0;

struct PBS_Plan* get_pbs_plan(void){
    printf(KERN_ALERT "[PBS_get_pbs_plan]: pbs_plan_address is %p, read %d times\n", pbs_plan_ptr, times_address_read);
    if (pbs_plan_ptr == NULL)
        printf(KERN_ALERT "[PBS_get_pbs_plan]: Plan is null\n");
    times_address_read++;
    return pbs_plan_ptr;
}
EXPORT_SYMBOL(get_pbs_plan);

/**
 * Is called by the tick-function after each timer interrupt
 * @param p
 */
void pbs_handle_prediction_failure(struct PBS_Plan *p) {
    long retired_instructions;
    assert(p->num_tasks < 400);
    if (p->cur_task->task_id == -2){
        if (LOG_PBS)
            printf(KERN_INFO "[PBS_SCHEDULE]%ld finished running p ticks", p->tick_counter);
        change_plan_state(p, PLAN_FINISHED);
        return;
    }

    printf(KERN_ERR "[PBS_pbs_handle_prediction_failure]%ld: ran tick, cur_task=%ld", p->tick_counter, p->cur_task->task_id);
    retired_instructions = get_retired_instructions();
    update_retired_instructions(retired_instructions, p);

    if(p->cur_task->state == PLAN_TASK_FINISHED){ // pbs_handle_prediction_failure() was called after a task has finished
        schedule_task_finished(p);
    } else {
        schedule_timer_tick(p);
    }
    p->tick_counter++;
}
EXPORT_SYMBOL(pbs_handle_prediction_failure);


/**
 * Is called when a task has finished; checks for t-2 and updates stats
 * @param p
 */
void schedule_task_finished(struct PBS_Plan *p){
    long lateness_cur_task;
    long instruction_surplus = p->cur_task->instructions_retired_slot - p->cur_task->instructions_real; // take surplus of instructions attributed to cur_task and remove them
    // --- check ---
    // todo: include other threshold-checks
    // check_tm2_node(p) && check_tm2_process(p->cur_process) &&
    if ( check_tm2_task(p)){
        //TODO: signal tm2
        signal_tm2(p);
        if (LOG_PBS)
            printf(KERN_WARNING "[PBS_schedule_task_finished]%ld: PBS_Task%ld finished early\n",p->tick_counter, p->cur_task->task_id);
    } else {
        if (LOG_PBS)
            printf(KERN_INFO "[PBS_schedule_task_finished]%ld: PBS_Task%ld finished, planned: %ld, real: %ld, retired: %ld\n", p->tick_counter, p->cur_task->task_id,
               p->cur_task->instructions_planned, p->cur_task->instructions_real, p->cur_task->instructions_retired_slot);
    }

    // --- update ---
    lateness_cur_task = p->cur_task->instructions_real - p->cur_task->instructions_planned;
    update_lateness_process(lateness_cur_task, p->cur_process);
    update_node_lateness(lateness_cur_task, p);

    p->instructions_retired -= instruction_surplus;
    p->cur_process->instructions_retired -= instruction_surplus;
    p->cur_task->instructions_retired_slot = p->cur_task->instructions_real;

    if (p->stress != 0)
        p->stress--;
    switch_task(p);
}

EXPORT_SYMBOL(schedule_task_finished);


/**
 * Function is called when a timer tick happened; check if t1,t2 is transgressed
 * @param p
 */
void schedule_timer_tick(struct PBS_Plan *p){
    long usable_buffer;
    //todo: test stress
    //todo: preemptions
    // --- check for t2 ---
    if(p->stress <= 0) {
        p->state = ON_PLAN;
        usable_buffer = calculate_usable_buffer(FREE_TIME, ASSIGNABLE_BUFFER, p->cur_process->buffer,
                                                     p->cur_process->length_plan,
                                                     p->cur_process->instructions_retired);
        if (check_t2_task(p) ||
            check_t2_process(p) ||
            check_t2_node(p))            {
            signal_t2(p);
        }
    }

    // --- check for t1 ---
    if (check_t1(p)){
        preempt_cur_task(p);
        return;
    }
    if(LOG_PBS)
        printf(KERN_INFO "[PBS_schedule_timer_tick]%ld: (%ld,%ld) retired instructions %ld\n",
           p->tick_counter, p->cur_task->task_id, p->cur_task->process_id, p->cur_task->instructions_retired_slot);
    if (p->stress)
        p->stress--;
}

EXPORT_SYMBOL(schedule_timer_tick);

/**
 * Implements a switching of tasks, changes the current Task for the Task in the next slot
 * @param p
 */
void switch_task(struct PBS_Plan* p){
    struct PBS_Task* old_task = p->cur_task;
    assert(p->cur_task->state == PLAN_TASK_FINISHED);
    p->tasks_finished++;
    p->index_cur_task++;
    p->cur_task++;
    if (p->cur_task->task_id == -1){
        handle_free_slot(p);
    }
    update_cur_task_process(p);
    if(LOG_PBS)
        printf(KERN_INFO "[PBS_switch_task]%ld: switched from task %ld to task %ld in tick %ld \n", p->tick_counter, old_task->task_id, p->cur_task->task_id, p->tick_counter);
}

EXPORT_SYMBOL(switch_task);

/**
 * starts a run inside the kernel with the given plan
 * @param p plan to start a run
 */
void start_run(struct PBS_Plan *p){
    while (p->state != PLAN_FINISHED){
        pbs_handle_prediction_failure(p);
    }
    printf(KERN_EMERG "[PBS_start_run]%ld: Plan finished!", p->tick_counter);
}
EXPORT_SYMBOL(start_run);

/**
 * Needs to be called when next task is free slot:
 * - Picks next task
 * - Updates Lateness
 * @param p
 */
void handle_free_slot(struct PBS_Plan* p){
    long lateness_node_before,  lateness_node_after;
    struct PBS_Task* free_slot;

    assert(p->cur_task->task_id == -1);
    assert(p->cur_process->process_id == -1);
    if (LOG_PBS)
        printf(KERN_INFO "[PBS_handle_free_slot]%ld: Found free slot of length %ld\n", p->tick_counter, p->cur_task->instructions_planned);
    lateness_node_before = p->lateness;
    free_slot = p->tasks;
    p->index_cur_task++;
    p->cur_task++;
    update_cur_task_process(p);
    update_retired_instructions(- free_slot->instructions_planned, p);
    lateness_node_after = p->lateness;
    assert(lateness_node_before > lateness_node_after);
}

EXPORT_SYMBOL(handle_free_slot);

// TODO: the following handle_unallocated_slot is the function that hsould be used; needs to be plugged in
/**
 * Is called if pbs encounters an unallocated time slot as next task to be executed
 * cur_task already pointing at free_slot
 * @param p the current plan
 */
void handle_unallocated_slot(struct PBS_Plan* p){
    int i = 0;
    long found_all_processes = 0;
    struct PBS_Task *cur_task = NULL;
    struct PBS_Task* next_task_to_run = NULL;
    struct PBS_Task next_tasks_each_process [MAX_NUMBER_PROCESSES] = {0};
    long max_lateness_process [MAX_NUMBER_PROCESSES] = {0};

    find_next_task_for_all_processes(p, i, found_all_processes, cur_task, next_tasks_each_process);


    move_task_in_plan(0, next_task_to_run, p);
    clear_preemption(next_task_to_run);
}

void find_next_task_for_all_processes(const struct PBS_Plan *p, int i, long found_all_processes, struct PBS_Task *cur_task,
                                 struct PBS_Task *next_tasks_each_process) {
    long cur_process_id = -2;
    // finds the next task for each process
    while (cur_task->task_id != -2 ||  found_all_processes == MAX_NUMBER_PROCESSES){
        cur_task = p->tasks+i;

        if (cur_process_id == -1) continue;
        cur_process_id = cur_task->process_id;
        if (next_tasks_each_process[cur_process_id].instructions_planned == 0){
            next_tasks_each_process[cur_process_id] = *cur_task;
            found_all_processes++;
        }
        i++;
    }
}
