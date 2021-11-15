//
// Created by ml on 23.03.21.
//


#ifndef LEVEL2_TASK_H


#define LEVEL2_TASK_H
// --- Task States ---
#define ON_TIME 0
#define TURNS_LATE 1
#define IS_LATE 2

void pbs_update_retired_instructions_task(long instructions, struct PBS_Task *task);
void change_task_state(struct PBS_Task *t, short state);
void clear_preemption(struct PBS_Task*);
short does_task_turn_late(long instructions_to_run, struct PBS_Task *task);

struct PBS_Task create_task(long task_id, long process_id, long instructions_planned, long instructions_real);


#endif //LEVEL2_TASK_H

