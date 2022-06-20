#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "config.h"
#include "pbs_entities.h"
#include "kernel_dummies.h"
#include "pb-scheduler.h"
#include "pmu_interface.h"
#include "plan.h"
#include "process.h"
#include "task.h"
#include "threshold_checking.h"
#include "prediction_failure_handling.h"
#include "prediction_failure_signaling.h"

struct PBS_Plan pbs_plan = {0};
struct PBS_Plan* pbs_plan_ptr = &pbs_plan;
static int times_address_read = 0;


/**
 * Is called by the tick-function after each timer interrupt
 * @param p
 */
void pbs_run_timer_tick(struct PBS_Plan *p) {
    long retired_instructions;
    assert(p->num_tasks < 400);
    if (p->cur_task->task_id == -2){
        if (LOG_PBS)
            printf(KERN_INFO "[pbs_run_timer_tick]%ld FINISHED\n", p->tick_counter);
        return;
    }
        if (LOG_PBS) {
            printf(KERN_INFO "[pbs_run_timer_tick]%ld: cur_task (%ld, %ld)\n", p->tick_counter,
                   p->cur_task->task_id, p->cur_task->process_id);
        }

    retired_instructions = get_retired_instructions();
    update_retired_instructions(retired_instructions, p);

    if(p->cur_task->state == PLAN_TASK_FINISHED){ // pbs_run_timer_tick() was called after a task has finished
        schedule_task_finished(p);
    } else {
        schedule_timer_tick(p);
    }
    p->tick_counter++;
}
EXPORT_SYMBOL(pbs_run_timer_tick);


/**
 * Is called when a task has finished; checks for t-2 and updates stats
 * @param p
 */
void schedule_task_finished(struct PBS_Plan *p){
    long lateness_cur_task;
    long instruction_surplus = p->cur_task->instructions_retired_slot - p->cur_task->instructions_real; // take surplus of instructions attributed to cur_task and remove them
    short tm2_task_transgressed, tm2_node_transgressed = 0;
    tm2_task_transgressed = check_tm2_task(p);
    tm2_node_transgressed = check_tm2_node(p);

    // --- check t-2 ---
    if ( tm2_task_transgressed || tm2_node_transgressed){
        signal_tm2(p);
        if (LOG_PBS)
            printf(KERN_WARNING "[schedule_task_finished]%ld: Task %ld finished early\n",p->tick_counter, p->cur_task->task_id);
    } else {
        if (LOG_PBS)
            printf(KERN_WARNING "[schedule_task_finished]%ld: Task %ld finished, planned: %ld, real: %ld, retired: %ld\n", p->tick_counter, p->cur_task->task_id,
                   p->cur_task->instructions_planned, p->cur_task->instructions_real, p->cur_task->instructions_retired_slot);
    }

    // --- update ---
    lateness_cur_task = p->cur_task->instructions_real - p->cur_task->instructions_planned;
    update_lateness_process(lateness_cur_task, p->cur_process);
    update_node_lateness(lateness_cur_task, p);

    p->instructions_retired -= instruction_surplus;
    p->cur_process->instructions_retired -= instruction_surplus;
    //p->cur_task->instructions_retired_slot = p->cur_task->instructions_real;

    if (p->stress != 0)
        p->stress--;
    switch_task(p);
}
EXPORT_SYMBOL(schedule_task_finished);


/**
 * Function is called when a timer tick happened; check if t1,t2 is transgressed
 * @param p
 */
void schedule_timer_tick(struct PBS_Plan *p){
    // --- check for t2 ---
    if(p->stress <= 0 || check_t2_preemptions(p->cur_task)) {
        if (check_t2_task(p) ||
            check_t2_process(p) ||
            check_t2_node(p) ){
            signal_t2(p);
        }
    }

    // --- check for t1 ---
    if (check_t1(p)){
        preempt_cur_task(p);
        if (p->cur_task->task_id == -1){
            handle_free_slot(p);
        }
        return;
    }
    if(LOG_PBS && 0)
        printf(KERN_INFO "[schedule_timer_tick]%ld: (%ld,%ld) retired/real:%ld/%ld\n",
               p->tick_counter, p->cur_task->task_id, p->cur_task->process_id, p->cur_task->instructions_retired_slot, p->cur_task->instructions_real);
    if (p->stress)
        p->stress--;
}
EXPORT_SYMBOL(schedule_timer_tick);

/**
 * Implements a switching of tasks, changes the current Task for the Task in the next slot
 * @param p
 */
void switch_task(struct PBS_Plan* p){
    struct PBS_Task* old_task = p->cur_task;
    assert(p->cur_task->state == PLAN_TASK_FINISHED);

    p->tasks_finished++;
    p->index_cur_task++;
    p->cur_task++;
    update_cur_process(p);
    if (p->cur_task->task_id == -1){
        handle_free_slot(p);
    }
    if(LOG_PBS)
        printf(KERN_INFO "[switch_task]%ld: switched from task %ld to task %ld in tick %ld \n", p->tick_counter, old_task->task_id, p->cur_task->task_id, p->tick_counter);
}
EXPORT_SYMBOL(switch_task);

/**
 * starts a run inside the kernel with the given plan
 * @param p plan to start a run
 */
void start_run(struct PBS_Plan *p){
    while (p->state != PLAN_FINISHED){
        pbs_run_timer_tick(p);
    }
    printf(KERN_EMERG "[PBS_start_run]%ld: Plan finished!", p->tick_counter);
}
EXPORT_SYMBOL(start_run);

/**
 * Needs to be called when next task is free slot:
 * - Picks next task
 * - Updates Lateness
 * - idling
 */
void handle_free_slot(struct PBS_Plan* p){
    long lateness_node_before,  lateness_node_after;
    struct PBS_Task* free_slot;

    assert(p->cur_task->task_id == -1);
    if (LOG_PBS)
        printf(KERN_INFO "[PBS_handle_free_slot]%ld: Found free slot of length %ld\n", p->tick_counter, p->cur_task->instructions_planned);

    lateness_node_before = p->lateness;
    free_slot = p->tasks;
    p->index_cur_task++;
    p->cur_task++;
    update_cur_process(p);
    update_retired_instructions(- free_slot->instructions_planned, p);
    lateness_node_after = p->lateness;
    assert(lateness_node_before > lateness_node_after);
    assert(p->cur_task->task_id != -1);
}
EXPORT_SYMBOL(handle_free_slot);

/**
 * Is called if pbs encounters an unallocated time slot as next task to be executed
 * cur_task already pointing at free_slot
 * @param p the current plan
 */
void handle_unallocated_slot(struct PBS_Plan* p){
    struct PBS_Task* next_task_to_run;
    struct PBS_Task next_tasks [MAX_NUMBER_PROCESSES] = {{0}};

    // stores upcoming task for each process in next_task_each_process
    find_next_task_for_all_processes(p, next_tasks);
    // set next_task_to_run to task that is preempted & most late
    next_task_to_run = find_substitution_task(next_tasks, p);

    replace_unallocated_slot_in_plan(next_task_to_run, p);
    clear_preemption(next_task_to_run);
}
EXPORT_SYMBOL(handle_unallocated_slot);

/**
 * Fills next_tasks with the next upcoming task according to the plan
 * If no next task can be found, it will be set to no_task_found
 * @param p current plan
 * @param next_tasks empty array that is to be filled with upcoming tasks, the index represents the pid
 */
void find_next_task_for_all_processes(const struct PBS_Plan *p, struct PBS_Task next_tasks [MAX_NUMBER_PROCESSES]) {
    const struct PBS_Task no_task_found = {.task_id = -1, .process_id=-1, .instructions_planned = -1};
    long cur_process_id = -3;
    long cur_task_id = -3;
    struct PBS_Task cur_task;
    int i = 0;
    cur_task = p->tasks[i];

    // mark all tasks as not found
    for (i = 0; i < MAX_NUMBER_PROCESSES; i++){
        if (next_tasks[i].process_id == -2) {
            break;
        }
        next_tasks[i] = no_task_found;
    }

    i = 0;
    // finds the next task for each process
    while (cur_task_id != -2){
        cur_task = p->tasks[i];
        cur_process_id = cur_task.process_id;
        cur_task_id = cur_task.task_id;

        // continue if legit task
        i++;
        if (cur_process_id == -1 || cur_process_id == -2) continue;

        // check if
        if (next_tasks[cur_process_id].instructions_planned == -1){
            next_tasks[cur_process_id] = cur_task;
        }
    }
    next_tasks[MAX_NUMBER_PROCESSES-1].task_id = -2;
}

/**
 * Searches all upcoming tasks for suitable candidate to run in unallocated time slot
 * @param next_tasks All upcoming tasks of active processes; index also determines pid; array is terminated with tid -2
 * @return Pointer to task that needs to be run next
 */
struct PBS_Task *find_substitution_task(struct PBS_Task next_tasks[MAX_NUMBER_PROCESSES], struct PBS_Plan *p) {
    long max_lateness = 0;
    long cur_lateness = 0;
    struct PBS_Task* cur_task = &next_tasks[0];
    struct PBS_Task* sub_task = NULL;

    // filter out processes whose next task was not preempted
    // if a task is filtered out its id will be set to -1
    while (cur_task->task_id != -2){

        if (cur_task->was_preempted == 0){
            cur_task->task_id = -1;
        }
        cur_task++;
    }

    // of all preempted tasks find that which process' is the latest
    cur_task = &next_tasks[0];
    while (cur_task->task_id != -2){
        if (cur_task->task_id != -1){
            cur_lateness = p->processes[cur_task->process_id].lateness;
            if (max_lateness < cur_lateness){
                max_lateness = cur_lateness;
                sub_task = cur_task;
            }
        }
        cur_task++;
    }

    // since next_task only contains copied values of the original task, the pointer has to refer to the original location in plan
    if (sub_task == NULL) return sub_task;
    else return find_task_with_task_id(p, sub_task->task_id);
}

struct PBS_Plan* get_pbs_plan(void){
    printf(KERN_ALERT "[PBS_get_pbs_plan]: pbs_plan_address is %p, read %d times\n", pbs_plan_ptr, times_address_read);
    if (pbs_plan_ptr == NULL)
        printf(KERN_ALERT "[PBS_get_pbs_plan]: Plan is null\n");
    times_address_read++;
    return pbs_plan_ptr;
}
EXPORT_SYMBOL(get_pbs_plan);

