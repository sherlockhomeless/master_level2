//
// Created by ml on 26.03.21.
//
#include <stdio.h>
#include<stdlib.h>

#include "pb-scheduler.h"
#include "pmu_interface.h"
#include "plan/plan.h"
#include "threshold_checking.h"
#include "config.h"
#include "prediction_failure_handling.h"

void update(long retired_instructions, Plan *p);
void schedule_task_finished(Plan*);
void schedule_timer_tick(Plan*);
void switch_task(Plan*);
void handle_free_slot(Plan*);


/**
 * Is called by the tick-function after each timer interrupt
 * @param p
 */
void schedule(Plan *p) {
    if (p->cur_task->task_id == -2){
        printf("[SCHEDULE]%ld finished running p ticks", p->tick_counter);
        change_plan_state(p, PLAN_FINISHED);
        return;
    }
    //todo: Just for checking:
    long retired_instructions = get_retired_instructions();
    update(retired_instructions, p);

    if(p->cur_task->state == TASK_FINISHED){ // schedule() was called after a task has finished
        schedule_task_finished(p);
    } else {
        schedule_timer_tick(p);
    }
    p->tick_counter++;
    if(LOG)
        printf("[SCHEDULE]%ld: (%ld,%ld) retired instructions %ld\n",
               p->tick_counter, p->cur_task->task_id, p->cur_task->process_id, p->cur_task->instructions_retired_slot);
}
/**
 * Updates ALL data structures to the state including the current tick
 * @param retired_instructions number of instructions retired on the execution of p-tasks since last tick
 * @param p current p
 */
void update(long retired_instructions, Plan* p){
    update_retired_instructions(retired_instructions, p);
    if(LOG)
        printf("[CUR_TASK]%ld: (%ld,%ld): instructions_retired_slot: %ld \n", p->tick_counter, p->cur_task->task_id, p->cur_task->process_id, p->cur_task->instructions_retired_slot);
}
/**
 * Is called when a task has finished; checks for t-2 and updates stats
 * @param p
 */
void schedule_task_finished(Plan *p){
    long lateness_cur_task;
    long instruction_surplus = p->cur_task->instructions_retired_slot - p->cur_task->instructions_real; // take surplus of instructions attributed to cur_task and remove them
    // --- check ---
    // todo: include other threshold-checks
    // check_tm2_node(p) && check_tm2_process(p->cur_process) &&
    if ( check_tm2_task(p->cur_task)){
        //TODO: signal tm2
        signal_tm2(p);
        printf("[SCHEDULE_TASK_FINISHED]%ld: Task %ld finished early\n",p->tick_counter, p->cur_task->task_id);
    } else {
        printf("[SCHEDULE_TASK_FINISHED]%ld: Task %ld finished, planned: %ld, real: %ld, retired: %ld\n", p->tick_counter, p->cur_task->task_id,
               p->cur_task->instructions_planned, p->cur_task->instructions_real, p->cur_task->instructions_retired_slot);
    }

    // --- update ---
    lateness_cur_task = p->cur_task->instructions_real - p->cur_task->instructions_planned;
    update_lateness_process(lateness_cur_task, p->cur_process);
    update_node_lateness(lateness_cur_task, p);

    p->instructions_retired -= instruction_surplus;
    p->cur_process->instructions_retired -= instruction_surplus;
    p->cur_task->instructions_retired_slot = p->cur_task->instructions_real;

    p->stress--;
    switch_task(p);
}


/**
 * Function is called when a timer tick happened; check if t1,t2 is transgressed
 * @param p
 */
void schedule_timer_tick(Plan *p){
    //todo: test stress
    //todo: preemptions


    // --- check for t2 ---
    if(p->stress <= 0) {
        p->state = ON_PLAN;
        long usable_buffer = calculate_usable_buffer(FREE_TIME, ASSIGNABLE_BUFFER, p->cur_process->buffer,
                                                     p->cur_process->length_plan,
                                                     p->cur_process->instructions_retired);
        if (check_t2_task(p->cur_task, p) ||
            check_t2_process(p->cur_process, usable_buffer) ||
            check_t2_node(p))            {
            signal_t2(p);
        }
    }

    // --- check for t1 ---
    if (check_t1(p)){
        preempt_cur_task(p);
        return;
    }
    p->stress--;
}


/**
 * Implements a switching of tasks
 * @param p
 */
void switch_task(Plan* p){
    Task* old_task = p->cur_task;
    p->tasks_finished++;
    p->tasks++;
    if (p->tasks->task_id == -1){
        handle_free_slot(p);
    }
    update_cur_task_process(p);
    if(LOG)
        printf("[SWITCH_TASK] switched from task %ld to task %ld in tick %ld \n", old_task->task_id, p->cur_task->task_id, p->tick_counter);
}

/**
 * Needs to be called when next task is free slot:
 * - Picks next task
 * - Updates Lateness
 * @param p
 */
void handle_free_slot(Plan* p){
    Task* free_slot = p->tasks;
    update_retired_instructions(- free_slot->instructions_planned, p);

}