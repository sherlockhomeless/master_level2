
#include "plan.h"

// --- Task States ---
#define ON_TIME 0
#define TURNS_LATE 1
#define IS_LATE 2


short tasks_turns_late(long, Task*);
/**
 * Tracks the state changes of a task in terms of lateness
 * @param instructions_to_run Instructions run on the task
 * @param task
 * @return Task-States, see in defines
 */
short tasks_turns_late(long instructions_to_run, Task* task){
    if (task->instructions_retired >= task->instructions_planned){
        return IS_LATE;
    }
    if (task->instructions_retired + instructions_to_run > task->instructions_planned)
        return TURNS_LATE;
    else
        return ON_TIME;
}

/**
 * Updates all data structures that are required for threshold tracking and calculating
 * @param instructions_retired PMU-Counter since it was last read
 * @param plan
 */
void update_retired_instructions(long instructions_retired, Plan* plan){
    short task_state = tasks_turns_late(instructions_retired, plan->cur_task);

    // --- update on tasks ---
    update_retired_instructions_task(instructions_retired, plan->cur_task);

    // --- update  instructions on process/plan ---
    update_retired_instructions_process(instructions_retired, plan->cur_process);
    plan->instructions_retired += instructions_retired;

    // --- update lateness on process/plan ---
    if (task_state != ON_TIME){
        if (task_state == IS_LATE){
            update_lateness_process(instructions_retired, plan->cur_process);
            plan->lateness += instructions_retired;
        } else {
            long latetess_dif = plan->cur_task->instructions_retired - plan->cur_task->instructions_planned;
            update_lateness_process(latetess_dif, plan->cur_process);
            plan->lateness += latetess_dif;
     }
    }
}

