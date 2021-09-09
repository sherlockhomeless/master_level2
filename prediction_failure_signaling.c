//
// Created by ml on 30.06.21.
//
#include <stdio.h>
#include <assert.h>


#include "kernel_dummies.h"
#include "pbs_entities.h"
#include "plan.h"
#include "prediction_failure_handling.h"
#include "prediction_failure_signaling.h"

struct PredictionFailureSignal lastSignals [SIZE_SIG_BUFFER]; // ring buffer to track latest signals
static int cur_sig_buffer_position;
void add_signal(struct PredictionFailureSignal sig);


void signal_t2(struct PBS_Plan* p){
    struct PredictionFailureSignal sig;
    change_plan_state(p, SIGNALED);
    if (LOG_PBS)
        printf(KERN_ALERT "[PBS_signal_t2]%ld: signaled prediction failure for process %ld", p->tick_counter, p->cur_process->process_id);
    p->stress = STRESS_RESET;

    sig.task_id = p->cur_task->task_id,
            sig.process_id = p->cur_process->process_id;
    sig.tick = p->tick_counter;
    sig.type_signal = STRETCH_SIGNAL;

    add_signal(sig);

    reschedule(p, STRETCH_SIGNAL);
}

EXPORT_SYMBOL(signal_t2);

void signal_tm2(struct PBS_Plan* p){
    struct PredictionFailureSignal  sig;
    change_plan_state(p, SIGNALED);
    if (LOG_PBS)
        printf(KERN_ALERT"[PBS_signal_tm2]%ld: signaled prediction failure for task %ld\n",p->tick_counter, p->cur_task->task_id);
    p->stress = STRESS_RESET;

    sig.task_id = p->cur_task->task_id,
            sig.process_id = p->cur_process->process_id;
    sig.tick = p->tick_counter;
    sig.type_signal = SHRINK_SIGNAL;

    add_signal(sig);

    reschedule(p, SHRINK_SIGNAL);
}

EXPORT_SYMBOL(signal_tm2);

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
    if (LOG_PBS)
        printf(KERN_DEBUG "[PBS_reschedule]%ld: Received %d signal and stretched/shrunk %ld tasks\n", p->tick_counter, signal, task_counter);
}

EXPORT_SYMBOL(reschedule);


/**
 *
 * @param sig
 */
void add_signal(struct PredictionFailureSignal sig){

    cur_sig_buffer_position++;
    if (cur_sig_buffer_position == 100)
        cur_sig_buffer_position = 0;
    lastSignals[cur_sig_buffer_position] = sig;
}

EXPORT_SYMBOL(add_signal);
/**
 * Returns latest signals in lastSignals
 * @param pick_signal Index of latest signal, 0 -> latest signal, 1 2nd latest signal,...
 * @return
 */
struct PredictionFailureSignal* get_pbs_signal(int pick_signal){
    assert(pick_signal >= 0 && pick_signal < SIZE_SIG_BUFFER);
    int target_index =  cur_sig_buffer_position - pick_signal;

    // no warp around happening
    if (target_index >= 0)
        return &lastSignals[target_index];
    // wrap around happens
    target_index = SIZE_SIG_BUFFER + target_index;
    return &lastSignals[target_index];
}
EXPORT_SYMBOL(get_pbs_signal);

void receive_new_plan(struct PBS_Plan* p){
    if (LOG_PBS)
        printf("[PBS_RECEIVE_NEW_PLAN]%ld: New Plan received\n", p->tick_counter);
}