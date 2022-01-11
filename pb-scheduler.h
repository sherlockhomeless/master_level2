//
// Created by ml on 26.03.21.
//

#ifndef LEVEL2_PB_SCHEDULER_H
#define LEVEL2_PB_SCHEDULER_H

struct PBS_Plan* get_pbs_plan(void);
void schedule_task_finished(struct PBS_Plan*);
void schedule_timer_tick(struct PBS_Plan*);
void switch_task(struct PBS_Plan*);
void handle_free_slot(struct PBS_Plan*);
struct PBS_Process* find_latest_process(struct PBS_Plan* p);
void idle(struct PBS_Plan*);
void pbs_run_timer_tick(struct PBS_Plan *p);
void start_run(struct PBS_Plan *p);
void handle_unallocated_slot(struct PBS_Plan*);
struct PBS_Task *find_substitution_task(struct PBS_Task[MAX_NUMBER_PROCESSES], struct PBS_Plan*);
void find_next_task_for_all_processes(const struct PBS_Plan*, struct PBS_Task[MAX_NUMBER_PROCESSES]);

#endif //LEVEL2_PB_SCHEDULER_H