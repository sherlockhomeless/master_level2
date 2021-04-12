
#include <stdio.h>
#include "plan.h"
#include "assert.h"

// --- Task States ---
#define ON_TIME 0
#define TURNS_LATE 1
#define IS_LATE 2

void update_free_space_usage(long, Plan*);
short does_task_turn_late(long instructions_to_run, Task *task);

/**
 * Tracks the state changes of a task in terms of lateness
 * @param instructions_to_run Instructions run on the task
 * @param task
 * @return Task-States, see in defines
 */
short does_task_turn_late(long instructions_to_run, Task* task){
    if (task->instructions_retired_slot >= task->instructions_planned){
        return IS_LATE;
    }
    if (task->instructions_retired_slot + instructions_to_run > task->instructions_planned)
        return TURNS_LATE;
    else
        return ON_TIME;
}

/**
 * Updates all data structures that are required for threshold tracking and calculating, also updates the state of a task accordingly
 * @param instructions_retired PMU-Counter since it was last read
 * @param p
 */
void update_retired_instructions(long instructions_retired, Plan* p){
    if (instructions_retired < 0){
        update_free_space_usage(instructions_retired, p);
    }

    Task * slot_owner;
    short task_state = does_task_turn_late(instructions_retired, p->cur_task);

    // --- update on tasks ---
    update_retired_instructions_task(instructions_retired, p->cur_task);

    // --- update retired instructions on slot ---
    if (p->cur_task->slot_owner == SHARES_NO_SLOT){
        p->cur_task->instructions_retired_slot += instructions_retired;
    } else {
        slot_owner = find_task_with_task_id(p, p->cur_task->slot_owner);

        slot_owner->instructions_retired_slot += instructions_retired;
    }

    // --- update  instructions on process/p ---
    update_retired_instructions_process(instructions_retired, p->cur_process);
    p->instructions_retired += instructions_retired;
}

/**
 * Updates the pointer cur_task & cur_process to reflect current state
 * @param p
 */
void update_cur_task_process(Plan *p) {
    Task old_cur_task = *p->cur_task;
    p->cur_task = p->tasks;
    change_task_state(p->cur_task, TASK_RUNNING);
    p->cur_process = &p->processes[p->cur_task->process_id];
    printf("[UPDATE_CUR_TASK]%ld: (%ld,%ld) is new cur_task\n", p->tick_counter,
           p->cur_task->task_id, p->cur_task->process_id);
}

/**
 * Helps find a task in p->tasks with given task_id
 * @param p
 * @param task_id
 * @return
 */
Task* find_task_with_task_id(Plan* p, long task_id){
    if (p->tick_counter == 188)
        show_tasks(p);
    Task* cur_task = p->cur_task;
    while (cur_task->task_id != -2){
        if (cur_task->task_id == task_id){
            assert(cur_task != NULL);
            return cur_task;
        }
        cur_task++;
    }
    return 0;
}
/**
 * Incorporates free space into lateness calculation
 * @param length_free_space
 * @param p
 */
void update_free_space_usage(long length_free_space, Plan* p){
    p->lateness -= length_free_space;
    if(LOG){
        printf("[UPDATE_FREE_SPACE_USAGE] Node-lateness changed from %ld to %ld", p->lateness + length_free_space, p->lateness );
    }
}

void update_node_lateness(long instructions, Plan* p){
    p->lateness += instructions;
}
void change_plan_state(Plan* p, short state){
    p->state = state;
    printf("[CHANGE_PLAN_STATE]%ld: changed state %d\n", p->tick_counter, p->state);
}

void show_tasks(Plan* p){
    Task task_list[400];
    for (int i = 0; i <= p->num_tasks; i++){
        task_list[i] = *(p->tasks + i);
    }
    printf("[SHOW_TASKS]%ld:", p->tick_counter);
    for (int i = 0; i<400; i++){
        printf("%d:(%ld, %ld) ", i, task_list[i].task_id, task_list[i].process_id);
    }
    printf("!\n");
}

void generate_test_plan(Plan *p, PlanProcess* process_list, Task *task_list) {
    p->tasks = task_list;
    p->cur_task = p->tasks;
    p->lateness= 0;
    p->state=ON_PLAN;
    p->stress= 0;

    for(int i = 0; i < MAX_NUMBER_PROCESSES; i++){
        p->processes[i] = process_list[i];
    }
}
