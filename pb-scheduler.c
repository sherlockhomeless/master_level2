//
// Created by ml on 26.03.21.
//
#include <stdio.h>
#include "pb-scheduler.h"
#include "pmu_interface.h"
#include "plan/plan.h"
#include "threshold_checking.h"
#include "config.h"
#include "prediction_failure_handling.h"

void update(long retired_instructions, Plan *plan);
void schedule_task_finished(Plan*);
void schedule_timer_tick(Plan*);
void switch_task(Plan*);
void handle_free_slot(Plan*);

/**
 * Is called by the tick-function after each timer interrupt
 * @param plan
 */
void schedule(Plan *plan) {
    if (plan->cur_task->task_id == -2){
        printf("[SCHEDULE] finished running plan in %ld ticks", plan->tick_counter);
        change_plan_state(plan, PLAN_FINISHED);
        return;
    }
    //todo: Just for checking:
    long retired_instructions = get_retired_instructions();
    update(retired_instructions, plan);

    if(plan->cur_task->state == TASK_FINISHED){ // schedule() was called after a task has finished
        schedule_task_finished(plan);
    } else {
        schedule_timer_tick(plan);
    }
    plan->tick_counter++;
    if(LOG)
        printf("[SCHEDULE] run tick %ld, cur_task %ld with retired instructions %ld\n",
               plan->tick_counter, plan->cur_task->task_id, plan->cur_task->instructions_retired_slot);
}
/**
 * Updates ALL data structures to the state including the current tick
 * @param retired_instructions number of instructions retired on the execution of plan-tasks since last tick
 * @param plan current plan
 */
void update(long retired_instructions, Plan* plan){
    update_retired_instructions(retired_instructions, plan);
    printf("cur_task: %ld: instructions_retired_slot: %ld \n", plan->cur_task->task_id, plan->cur_task->instructions_retired_slot);
}
/**
 * Is called when a task has finished; checks for t-2 and updates stats
 * @param p
 */
void schedule_task_finished(Plan *p){
    long lateness_cur_task;
    long instruction_surplus = p->cur_task->instructions_retired_slot - p->cur_task->instructions_real; // take surplus of instructions attributed to cur_task and remove them
    // --- check ---
    if (check_tm2_node(p) && check_tm2_process(p->cur_process) && check_tm2_task(p->cur_task)){
        //TODO: signal tm2
        printf("Task %ld finished early", p->cur_task->task_id);
    } else {
        printf("[SCHEDULE_TASK_FINISHED] Task %ld finished, planned: %ld, real: %ld, retired: %ld\n", p->cur_task->task_id,
               p->cur_task->instructions_planned, p->cur_task->instructions_real, p->cur_task->instructions_retired_slot);
    }

    // --- update ---
    lateness_cur_task = p->cur_task->instructions_real - p->cur_task->instructions_planned;
    update_lateness_process(lateness_cur_task, p->cur_process);
    update_node_lateness(lateness_cur_task, p);

    p->instructions_retired -= instruction_surplus;
    p->cur_process->instructions_retired -= instruction_surplus;
    p->cur_task->instructions_retired_slot = p->cur_task->instructions_real;
    switch_task(p);
}


/**
 * Function is called when a timer tick happened; check if t1,t2 is transgressed
 * @param p
 */
void schedule_timer_tick(Plan *p){

    // --- check for t2 ---
    long usable_buffer = calculate_usable_buffer(FREE_TIME, ASSIGNABLE_BUFFER, p->cur_process->buffer, p->cur_process->length_plan,
                                                 p->cur_process->instructions_retired );
    if (check_t2_task(p->cur_task) || check_t2_process(p->cur_process, usable_buffer) || check_t2_node(p)){
        signal_t2(p);
    }

    // --- check for t1 ---
    if (check_t1(p)){
        preempt_cur_task(p);
        return;
    }
}


/**
 * Implements a switching of tasks
 * @param p
 */
void switch_task(Plan* p){
    Task* old_task = p->cur_task;
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