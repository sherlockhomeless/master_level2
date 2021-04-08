//
// Created by ml on 26.03.21.
//
#include <stdio.h>

#include "threshold_checking.h"
#include "plan/plan.h"
#include "config.h"

long calculate_t1(Task*);
long calculate_t2_task(Task*);

long calculate_t1(Task* task){
    long t1_relative, t1_minimum, t1_maximum, t1;
    t1_minimum = task->instructions_planned + NO_PREEMPTION;
    // todo: problematic conversion
    t1_relative = (long) (task->instructions_planned * SIGMA_T1);
    t1_maximum = task->instructions_planned + NO_PREEMPTION;
    if(0)
        printf("[t1] task %ld: instructions_planned=%ld, t1_min=%ld, t1_relative=%ld, t1_max=%ld\n", task->task_id,
               task->instructions_planned, t1_minimum, t1_relative, t1_maximum);
    if (t1_minimum > t1_relative)
        t1_relative = t1_minimum;
    if (t1_maximum > t1_relative) {
        t1 = t1_relative;
    } else {
        t1 = t1_maximum;
    }
    return t1;
}

short check_t1(Plan* p) {
    Task* task_to_check;
    //task was moved and is not in original slot anymore
    if (p->cur_task->slot_owner == SHARES_NO_SLOT){
        //todo: stack check, is >1 one task in new slot
        task_to_check = p->cur_task;
    } else {
        task_to_check =  find_task_with_task_id(p, p->cur_task->slot_owner);
    }
    long t1 = calculate_t1(task_to_check);
    if(task_to_check->instructions_retired_slot >= t1){
        return T1;
    } else {
        return OK;
    }
}

long calculate_t2_task(Task* task){
    long t2_task, t2_task_min, t2_task_relative, t1;
    t1 = calculate_t1(task);
    t2_task_min = (long) (t1 + T2_SPACER);
    t2_task_relative = (long) (task->instructions_planned * CAP_LATENESS);
    if(0)
        printf("[t2_task] task %ld: instructions_planned=%ld, t2_task_min=%ld, t2_task_relative=%ld\n",
               task->task_id, task->instructions_planned, t2_task_min, t2_task_relative);

    if (t2_task_min > t2_task_relative)
        t2_task = t2_task_min;
    else
        t2_task = t2_task_relative;

    return t2_task;

}
short check_t2_task(Task* task){
    long t2_task = calculate_t2_task(task);
    // compare
    if (task->instructions_retired_slot >= t2_task)
        return T2;
    else
        return OK;
}
short check_tm2_task(Task* task){
    long tm2_task = calculate_t2_task(task) * -2;
    if (task->instructions_retired_slot < tm2_task && task->state == TASK_FINISHED)
        return TM2;
    else
        return OK;
}

short check_t2_process(PlanProcess *process, long usable_buffer) {
    // TODO: HIER WEITER
    return OK;
}
short check_tm2_process(PlanProcess * process) {

    return OK;
}

/**
 * Calculates the usable_buffer, float-values have to be given as integers because of kernel limitations
 * @param free_time freetime as integer, eg. 10 => 10% free time
 * @param assignable_buffer
 * @param buffer
 * @param length_process_plan
 * @param length_process_finished
 * @return usable_buffer in instructions
 */
long calculate_usable_buffer(int free_time, int assignable_buffer, long buffer, long length_process_plan, long length_process_finished){
    long buffer_with_free_time_applied;
    long available_buffer;
    long usable_buffer;
    long process_progress;
    long const accuracy = 10000; // No floating point in Kernel, therefore accuracy provides shifting of ratio for

    buffer_with_free_time_applied =  (buffer * free_time / 100) ;
    available_buffer = buffer_with_free_time_applied * assignable_buffer / 100;
    process_progress = (length_process_finished * accuracy) / length_process_plan;

    usable_buffer = available_buffer * process_progress /accuracy;

    return usable_buffer;
}

short check_t2_node(Plan* plan){

    return OK;
}
short check_tm2_node(Plan* plan){

    return OK;
}
