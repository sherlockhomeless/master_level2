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

/* --- T1 ----*/
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

/* --- T2 ----*/
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
            allowed_plan_buffer = allowed_plan_buffer + (p->stress * T2_STRESS_GAIN) - RESCHEDULE_TIME;
            allowed_plan_buffer = allowed_plan_buffer < T2_PROCESS_MINIMUM ? T2_PROCESS_MINIMUM : allowed_plan_buffer;
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

short check_t2_node(struct PBS_Plan* plan){
    long t2_node = calculate_t2_node(plan);
    if (!T2_NODE_ENABLED)
        return OK;

    if (plan->lateness > t2_node){
        return T2;
    } else {
        return OK;
    }
}
EXPORT_SYMBOL(check_t2_node);


long calculate_t2_node(struct PBS_Plan* p){
    long t2_node_min = p->num_processes * T2_PROCESS_MINIMUM;
    long t2_node_relative = ((p->instructions_planned * T2_NODE_LATENESS)/100) + (p->stress * T2_STRESS_GAIN) - RESCHEDULE_TIME;
    long t2_node = t2_node_relative < t2_node_min ? t2_node_min : t2_node_relative;
    return t2_node;
}
EXPORT_SYMBOL(calculate_t2_node);

/* --- T-2 ----*/
/**
 * @param p
 * @return
 */
short check_tm2_task(struct PBS_Plan* p){
    long tm2_task;
    struct PBS_Task* task = p->cur_task;
    long plan_length = task->instructions_planned;

    if (!TM2_TASK_ENABLED) {
        return OK;
    }

    tm2_task = calculate_tm2_task(task);

    assert(tm2_task < plan_length);
    assert(tm2_task > 0);

    if (task->instructions_retired_slot > tm2_task && task->state == PLAN_TASK_FINISHED)
        return TM2;
    else
        return OK;
}
EXPORT_SYMBOL(check_tm2_task);

/**
 * Calculates Tm2; consider the fact that MAX/MIN values are swapped for tm2-values
 */
long calculate_tm2_task(struct PBS_Task* t){
    long tm2_task_relative, tm2_task_max, tm2_task;
    tm2_task_relative = (t->instructions_planned * TM2_SIGMA)/100;
    tm2_task_max = tm2_task_relative > TM2_TASK_SIGNALING_LIMIT ? t->instructions_planned + TM2_TASK_SIGNALING_LIMIT : tm2_task_relative;
    tm2_task_max = tm2_task_max > 0 ? tm2_task_max : 1; // in some configurations tm2 could be 0 or smaller
    tm2_task = tm2_task_max < TM2_TASK_SIGNALING_START ? TM2_TASK_SIGNALING_START : tm2_task_max;
    return tm2_task;
}
EXPORT_SYMBOL(calculate_tm2_task);

long calculate_tm2_node(struct PBS_Plan* p){
    assert(0);
}

short check_tm2_node(struct PBS_Plan* plan){
    if (!TM2_NODE_ENABLED)
        return OK;
    return OK;
}
EXPORT_SYMBOL(check_tm2_node);

short check_t2_preemptions(struct PBS_Task *t) {
    if (!T2_PREEMPTIONS_ENABLED)
        return OK;

    if (t->was_preempted > T2_MAX_PREEMPTIONS){
        return T2;
    } else {
        return OK;
    }
}
EXPORT_SYMBOL(check_t2_preemptions);