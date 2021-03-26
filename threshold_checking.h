//
// Created by ml on 23.03.21.
//

#include "plan/plan.h"
#ifndef LEVEL2_THRESHOLDS_H
#define LEVEL2_THRESHOLDS_H

#endif //LEVEL2_THRESHOLDS_H
#define OK 0b0
#define T1 0b1
#define T2 0b10
#define TM2 0b11


short check_t1(Task*);
short check_t2_task(Task*);
short check_tm2_task(Task*);

short check_t2_process(PlanProcess *);
short check_tm2_process(PlanProcess *);

short check_t2_node(Plan*);
short check_tm2_node(Plan*);



