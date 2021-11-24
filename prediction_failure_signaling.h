//
// Created by ml on 30.06.21.
//

#ifndef LEVEL2_PREDICTION_FAILURE_SIGNALING_H
#define LEVEL2_PREDICTION_FAILURE_SIGNALING_H

struct PredictionFailureSignal{
    long task_id;
    long process_id;
    long tick;
    short type_signal;
};

void receive_new_plan(struct PBS_Plan*);
void reschedule(struct PBS_Plan *p, short signal, long target_pid);
struct PredictionFailureSignal* get_pbs_signal(int pick_signal);
void signal_t2(struct PBS_Plan* p);
void signal_tm2(struct PBS_Plan* p);
void print_signals();
int number_prediction_failures_caused();
#endif //LEVEL2_PREDICTION_FAILURE_SIGNALING_H
