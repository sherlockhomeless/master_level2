//
// Created by ml on 25.03.21.
//

#include "process.h"

/**
 *
 * @param instructions instructions run on the process given
 */
void update_retired_instructions_process(long instructions, PlanProcess* process){
    process->instructions_retired += instructions;
}


void update_lateness_process(long late_instructions, PlanProcess* process){
    process->lateness += late_instructions;
}

short is_process_late(PlanProcess* process){
    if(process->lateness < 0)
        return 0;
    else
        return 1;
}