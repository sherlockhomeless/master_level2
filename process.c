//
// Created by ml on 25.03.21.
//
#include <assert.h>
#include "kernel_dummies.h"

#include "pbs_entities.h"
#include "process.h"

/**
 *
 * @param instructions instructions run on the process given
 */
void update_retired_instructions_process(long instructions, struct PBS_Process* process){
    process->instructions_retired += instructions;
}

EXPORT_SYMBOL(update_retired_instructions_process);


void update_lateness_process(long late_instructions, struct PBS_Process* process){
    process->lateness += late_instructions;
}

EXPORT_SYMBOL(update_lateness_process);

short is_process_late(struct PBS_Process* process){
    if(process->lateness < 0)
        return 0;
    else
        return 1;
}
EXPORT_SYMBOL(is_process_late);

void update_finished_tasks_in_process(struct PBS_Process* process){
    process->num_tasks_remaining--;
}
EXPORT_SYMBOL(update_finished_tasks_in_process);

struct PBS_Process create_process(long pid, long num_tasks, long buffer, long length_plan){
    struct PBS_Process new_process = {pid, num_tasks, buffer, 0, length_plan, 0};
    return new_process;

}
EXPORT_SYMBOL(create_process);

/**
 * @param process
 * @return an integer between 0 and 100;
 */
int calculate_process_completion(struct PBS_Process* process){
    int completion;
    long process_completion = (process->instructions_retired*100) / process->length_plan;
    completion = process_completion > 100 ? 100 : (int) process_completion;
    assert(process_completion >= 0);

    return completion;
}
EXPORT_SYMBOL(calculate_process_completion);
