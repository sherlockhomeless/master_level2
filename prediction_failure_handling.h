//
// Created by ml on 26.03.21.
//

#include <stdio.h>
#include "plan/plan.h"

#ifndef LEVEL2_PREDICTION_FAILURE_HANDLING_H
#define LEVEL2_PREDICTION_FAILURE_HANDLING_H

#define STRETCH_SIGNAL 0
#define SHRINK_SIGNAL 1

#define SIZE_SIG_BUFFER 100
#endif //LEVEL2_PREDICTION_FAILURE_HANDLING_H

typedef struct {
    short type_signal; // either stretch or shrink
    long tick; // tick at which signal was send
    long cur_task_id; // current task at tick time
    long cur_process_id; // current process at signal time
} PredictionFailureSignal;


void add_signal(PredictionFailureSignal sig); // adds a signal to the ring buff
PredictionFailureSignal* get_signal(int pick_signal); //rea

void signal_t2(struct PBS_Plan*);
void signal_tm2(struct PBS_Plan*);
void preempt_cur_task(struct PBS_Plan*);




// --- for testing ---
int get_stack_size_preempted_tasks(struct PBS_Task *tasks_to_move, struct PBS_Plan *p);
long find_slot_to_move_to(long target_pid, struct PBS_Plan* p);
void move_preempted_tasks(long insertion_slot, int stack_size, struct PBS_Task*preempted_tasks, struct PBS_Task *p);
void move_other_tasks_forward(long insertion_slot, long stack_size, struct PBS_Plan* p);