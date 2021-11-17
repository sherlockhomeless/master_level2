//
// Created by ml on 26.03.21.
//
#include <stdio.h>
#include <assert.h>

#include "kernel_dummies.h"

#include "pbs_entities.h"
#include "plan.h"
#include "process.h"
#include "threshold_checking.h"
#include "config.h"


long calculate_t1(struct PBS_Task*);
long calculate_t2_task( struct PBS_Plan *p);

long calculate_t1(struct PBS_Task* task){
    long t1_relative, t1_minimum, t1_maximum, t1;
    t1_minimum = task->instructions_planned + T1_NO_PREEMPTION;
    t1_relative = task->instructions_planned * T1_SIGMA;
    t1_maximum = task->instructions_planned + T1_NO_PREEMPTION;
    if(LOG_PBS)
        printf("[calculate_t1] task %ld: instructions_planned=%ld, t1_min=%ld, t1_relative=%ld, t1_max=%ld\n", task->task_id,
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

EXPORT_SYMBOL(calculate_t1);

short check_t1(struct PBS_Plan* p) {
    struct PBS_Task* task_to_check;
    long t1;

    if(!T1_ENABLED) {
        return OK;
    }
    //task was moved and is not in original slot anymore
    if (p->cur_task->slot_owner == p->cur_task->task_id){
        //todo: stack check, is >1 one task in new slot
        task_to_check = p->cur_task;
    } else {
        task_to_check =  find_task_with_task_id(p, p->cur_task->slot_owner);
    }
     t1 = calculate_t1(task_to_check);
    if(task_to_check->instructions_retired_slot >= t1){
        return T1;
    } else {
        return OK;
    }
}
EXPORT_SYMBOL(check_t1);

long calculate_t2_task(struct PBS_Plan *p) {
    struct PBS_Task* task = p->cur_task;
    long t2_task, t2_task_min, t2_task_relative, t1;
    t1 = calculate_t1(task);
    t2_task_min = (long) (t1 + T2_SPACER);
    t2_task_relative = (long) (task->instructions_planned * T2_SIGMA);
    if(LOG_PBS)
        printf(KERN_INFO "[PBS_calculate_t2_task] task %ld: instructions_planned=%ld, t2_task_min=%ld, t2_task_relative=%ld\n",
               task->task_id, task->instructions_planned, t2_task_min, t2_task_relative);

    if (t2_task_min > t2_task_relative)
        t2_task = t2_task_min;
    else
        t2_task = t2_task_relative;

    return t2_task;
}

EXPORT_SYMBOL(calculate_t2_task);

short check_t2_task( struct PBS_Plan *p) {
    long t2_task;
    if(!T2_TASK_ENABLED)
        return OK;
    t2_task = calculate_t2_task(p);
    // compare
    if (p->cur_task->instructions_retired_slot >= t2_task)
        return T2;
    else
        return OK;
}
EXPORT_SYMBOL(check_t2_task);

/**
 * Check tm2, a prediction failure condition related to significant earliness
 * @param p
 * @return
 */
short check_tm2_task(struct PBS_Plan* p){
    struct PBS_Task* task = p->cur_task;
    long plan_length = task->instructions_planned;
    long tm2_task = (plan_length * T2_SIGMA / 10) / 100;

    if (!TM2_TASK_ENABLED) {
        return OK;
    }
    assert(tm2_task < plan_length);

    if (task->instructions_retired_slot < tm2_task && task->state == PLAN_TASK_FINISHED)
        return TM2;
    else
        return OK;
}

EXPORT_SYMBOL(check_tm2_task);

/**
 * Implements the 2-level check for t2_process
 * @param p
 * @return
 */
short check_t2_process(struct PBS_Plan* p) {
    long capacity_buffer, allowed_plan_buffer;
    struct PBS_Process* cur_process = p->cur_process;
    if (!T2_PROCESS_ENABLED)
        return OK;
    else  {
        capacity_buffer = calculate_capacity_buffer(cur_process);
        if(cur_process->lateness > capacity_buffer){
            allowed_plan_buffer = calculate_allowed_plan_buffer(cur_process, p);
            if (cur_process->lateness > allowed_plan_buffer){
                return T2;
            } else {
                return OK;
            }
        } else {
            return OK;
        }
    }
}
EXPORT_SYMBOL(check_t2_process);

long calculate_capacity_buffer(struct PBS_Process *process) {
    long capacity_buffer = process->length_plan * T2_CAPACITY_BUFFER;

    assert(capacity_buffer > process->length_plan);

    return process->length_plan * T2_CAPACITY_BUFFER;
}

long calculate_allowed_plan_buffer(struct PBS_Process* process, struct PBS_Plan* p){
    int process_completion = calculate_process_completion(process);
    long plan_buffer = process->buffer;
    long capacity_buffer_deadline = (plan_buffer * T2_CAPACITY_BUFFER) - plan_buffer;
    long capacity_buffer_per_process = capacity_buffer_deadline / p->num_processes;
    long cleared_plan_buffer = capacity_buffer_per_process * T2_ASSIGNABLE_PLAN_BUFFER;
    long allowed_plan_buffer = (cleared_plan_buffer * process_completion)/100;
    return allowed_plan_buffer;
}


EXPORT_SYMBOL(check_t2_process);

short check_tm2_process(struct PBS_Plan* p) {
    if (!TM2_PROCESS_ENABLED)
        return OK;
    return OK;
}
EXPORT_SYMBOL(check_tm2_process);


short check_t2_node(struct PBS_Plan* plan){
    if (!T2_NODE_ENABLED)
        return OK;
    return OK;
}

EXPORT_SYMBOL(check_t2_node);

short check_tm2_node(struct PBS_Plan* plan){
    if (!TM2_NODE_ENABLED)
        return OK;
    return OK;
}

EXPORT_SYMBOL(check_tm2_node);
