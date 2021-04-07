//
// Created by ml on 23.03.21.
//

#ifndef LEVEL2_PROCESS_H
#define LEVEL2_PROCESS_H

#endif //LEVEL2_PROCESS_H

typedef struct {
    long process_id;
    long num_tasks;
    long buffer;
    long lateness;
    long length_plan;
    long instructions_retired;
} PlanProcess;

void update_retired_instructions_process(long instructions, PlanProcess* process);
void update_lateness_process(long late_instructions, PlanProcess* process);
short is_process_late(PlanProcess*);