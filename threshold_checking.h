//
// Created by ml on 23.03.21.
//

#include "plan.h"
#include "config.h"


#define OK 0b0
#define T1 0b1
#define T2 0b10
#define TM2 0b11

short check_t1(struct PBS_Plan*);
short check_t2_task( struct PBS_Plan *p);
short check_tm2_task(struct PBS_Plan *p);

short check_t2_process(struct PBS_Plan*);
short check_tm2_process(struct PBS_Plan *);
long calculate_capacity_buffer(struct PBS_Process *process);
long calculate_allowed_plan_buffer(struct PBS_Process *process, struct PBS_Plan *p);

long calculate_t2_node(struct PBS_Plan*);
short check_t2_node(struct PBS_Plan*);
short check_tm2_node(struct PBS_Plan*);

short check_t2_preemptions(struct PBS_Plan*);

