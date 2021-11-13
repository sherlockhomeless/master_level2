//
// Created by ml on 23.03.21.
//
/**
 * todo: nice_to_have_
 */

 // todo: more assertsbuffer=buffer=
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pbs_entities.h"
#include "userland_only_helper.h"
#include "plan.h"
#include "task.h"
#include "pb-scheduler.h"
#include "threshold_checking.h"
#include "prediction_failure_handling.h"


const char* PLAN_PATH = "/home/ml/Dropbox/Master-Arbeit/code/level2/test/plan.log";
const char* BINARY_PATH = "/home/ml/Dropbox/Master-Arbeit/code/lkm/pbs_plan_copy/write_plan";

void read_plan(FILE*, char* , long);
long get_file_size(FILE*);

void test_plan_parsing(struct PBS_Plan*);
void check_thresholds(struct PBS_Plan*);
void check_run_task_on_time(struct PBS_Plan *plan);
void check_run_task_early_time(struct PBS_Plan *plan);
void check_run_task_tm2_early_time(struct PBS_Plan *plan);
void check_run_task_late_time(struct PBS_Plan *plan);
void check_preempt_task(struct PBS_Plan *plan);
void check_signal_t2_task(struct PBS_Plan *plan);

void run_unit_tests();
void test_find_slot_to_move_to();
void test_move_others();
void test_insert_preempted_tasks();
void test_task_moving();
void test_handle_unallocated();
void test_find_next_task_for_all_processes();
void test_find_suitable_task();
void test_replace_unallocated_slot_in_plan();
int test_run();

struct PBS_Task * run(struct PBS_Plan *p, struct PBS_Task* t);


int main(){
    run_unit_tests();
    return test_run();
}

// #### UNIT TESTS ####
void run_unit_tests(){
    test_find_slot_to_move_to();
    test_move_others();
    test_task_moving();
    test_insert_preempted_tasks();
    test_find_next_task_for_all_processes();
    test_find_suitable_task();
    test_replace_unallocated_slot_in_plan();
    test_handle_unallocated();
}

void test_find_slot_to_move_to(){
    int i;
    struct PBS_Plan p = {0};
    fill_empty_test_plan(&p);
    struct PBS_Process processes [MAX_NUMBER_PROCESSES];

    long order[5] = {0,1,2,3,0};
    long index;

    for ( i = 0; i < 5; i++){
        p.tasks[i].process_id = order[i];
    }

    index = find_slot_to_move_to(0, &p );
    assert(index == 3);

    printf("passed test_find_slot_to_move_to()\n");
}

void test_move_others(){
    int i;
    struct PBS_Plan p = {0};
    fill_empty_test_plan(&p);
    struct PBS_Task* tasks = &p.tasks[0];
    long order[5] = {0,1,2,3,4};
    for( i = 0; i < 5; i++){
        tasks[i].process_id = order [i];
        tasks[i].task_id = order[i];
        tasks[i].slot_owner = SHARES_NO_SLOT;
    }

    move_other_tasks_forward(4, 1, &p);
    assert(tasks[0].task_id == 1);
    assert(tasks[3].task_id == 4);


    for ( i = 0; i < 5; i++){
        tasks[i].task_id = order[i];
    }

    move_other_tasks_forward(5, 2, &p);
    assert(tasks[0].task_id == 2);
    assert(tasks[1].task_id == 3);

    printf("passed test_move_others()\n");
}

void test_task_moving(){
    long first_task_id;
    long second_task_id;
    struct PBS_Plan* plan = (struct PBS_Plan*) malloc(sizeof(struct PBS_Plan));
    test_plan_parsing(plan);

    plan->tasks[0].slot_owner = plan->tasks[1].task_id;
    plan->tasks[1].process_id = 0;
    preempt_cur_task(plan);
}

void test_insert_preempted_tasks() {
    int i;
    struct PBS_Plan p = {0};
    fill_empty_test_plan(&p);
    struct PBS_Task* tasks = &p.tasks[0];

    long order[5] = {0,1,2,3,4};
    for( i = 0; i < 5; i++){
        tasks[i].process_id = order [i];
        tasks[i].task_id = order[i];
    }

    // 0-1-2-3-4 > 0-1-2-0-4
    move_preempted_tasks(3,1,tasks, &p);

    assert(tasks[3].task_id == 0);
    assert(tasks[3].slot_owner == 4);
    assert(tasks[4].task_id == 4);
    assert(tasks[2].task_id == 2);

    printf("passed test_insert_preempted_tasks()\n");
}

void test_find_next_task_for_all_processes(){
    struct PBS_Plan p = {0};
    struct PBS_Task next_tasks [MAX_NUMBER_PROCESSES];
    short has_task_with_termination_id = 0;
    int i;

    // idle slot
    p.tasks[0] = create_task(-1, -1, 100, 100);
    // next task for p0
    p.tasks[1] = create_task(0,0,10,0);
    p.tasks[6] = create_task(20, 0, 10, 0);
    // next task p1
    p.tasks[2] = create_task(1,1,10,0);
    p.tasks[5] = create_task(10,1,10,0);
    // delimiter
    p.tasks[50] = create_task(-2,0,0,0);

    find_next_task_for_all_processes(&p, next_tasks);

    assert(next_tasks[0].task_id == 0);
    assert(next_tasks[1].task_id == 1);

    for (i = 0; i < MAX_NUMBER_PROCESSES; i++){
        if (next_tasks[i].task_id == -2){
            has_task_with_termination_id++;
        }
    }
    assert(has_task_with_termination_id == 1);
    printf("passed test_find_next_task_for_all_processes()\n");
}


void test_find_suitable_task(){
    struct PBS_Plan p = {0};
    struct PBS_Task next_tasks[MAX_NUMBER_PROCESSES];
    struct PBS_Task *next_task;
    p.tasks[0] = create_task(0, 0, 1000, 2000);
    p.tasks[0].was_preempted = 1;
    p.processes[0].lateness = -100;

    p.tasks[1] = create_task(1, 1, 1000, 2000);
    p.tasks[1].was_preempted = 2;
    p.processes[1].lateness = 200;

    p.tasks[2] = create_task(2,2,200, 200);
    p.tasks[2].was_preempted = 0;
    p.processes[2].lateness = 100;

    p.tasks[3].task_id= -2;

    find_next_task_for_all_processes(&p, next_tasks);
    next_task = find_substitution_task(next_tasks, &p);

    assert(next_task->task_id == 1);
    printf("passed test_find_suitable_task()\n");
}

void test_replace_unallocated_slot_in_plan(){
    struct PBS_Plan p = {0};

    p.tasks[0] = create_task(-1, -1, 10, 10);
    p.tasks[1] = create_task(1, 1, 10, 10);
    p.tasks[2] = create_task(2, 2, 10, 10);
    p.tasks[3] = create_task(3, 3, 10, 10);
    p.tasks[4] = create_task(-2, -2, 0, 0);


    replace_unallocated_slot_in_plan(&p.tasks[0], &p);

    assert(p.tasks[0].task_id == 1);
    assert(p.tasks[1].task_id == 2);
    assert(p.tasks[2].task_id == 3);
    assert(p.tasks[3].task_id == -2);
    printf("passed test_replace_unallocated_slot_in_plan()\n");

}

void test_handle_unallocated(){
    int i;
    struct PBS_Plan p = {0};
    struct PBS_Task* tasks = &p.tasks[0];
    struct PBS_Task cur_t;
    fill_empty_test_plan(&p);


    // unallocated slot
    cur_t = create_task(-1, -1, 100, 100);
    p.tasks[0] = cur_t;

    // first task: pid 0, preempted 1, p-lateness 50
    cur_t = create_task(0,0, 100, 100);
    cur_t.was_preempted = 1;
    p.processes[0].lateness = 50;
    p.tasks[1] = cur_t;

    // second task: pid 1, preempted 1, p-lateness 100
    cur_t = create_task(1,1, 100, 100);
    cur_t.was_preempted = 1;
    p.tasks[2] = cur_t;
    p.processes[1].lateness = 100;

    // thirst task: pid 2, preempted 0, p-lateness 200
    cur_t = create_task(2, 2, 100, 100);
    cur_t.was_preempted = 0;
    p.tasks[3] = cur_t;
    p.processes[2].lateness = 200;

    // terminate list of valid tasks
    cur_t = create_task(-2, -2, -2, -2);
    p.tasks[4] = cur_t;
    handle_unallocated_slot(&p);

    assert(p.tasks[0].process_id == 1);
    assert(p.tasks[1].process_id == 0);
    assert(p.tasks[2].process_id == 2);

    printf("passed test_handle_unallocated()\n");
}

// ### TEST RUN ###
int test_run(){
    int i;
    struct PredictionFailureSignal*  sig;
    struct PBS_Task* cur_task;
    struct PBS_Plan plan = {0};
    struct PBS_Plan* plan_ptr = &plan;
    fill_empty_test_plan(plan_ptr);
    test_plan_parsing(plan_ptr);
    check_thresholds(plan_ptr);
    check_run_task_on_time(plan_ptr); // finish t0
    check_run_task_early_time(plan_ptr); // finish t1
    check_run_task_tm2_early_time(plan_ptr); // finish t2
    check_run_task_late_time(plan_ptr);
    check_preempt_task(plan_ptr);
    while(plan_ptr->state != PLAN_FINISHED) {
        pbs_handle_prediction_failure(plan_ptr);
    }

    for ( i = 0; i < 3; i++){
      /*  sig = get_pbs_signal(i);
        //FIXME: Signals are trash
        printf("signal: tick=%ld type=%d, task=%ld, process=%ld\n",
               sig->tick, sig->type_signal, sig->task_id, sig->process_id);*/
    }
    return 0;
}
// t0
void check_run_task_on_time(struct PBS_Plan* plan){
    int i;
    long ins_per_tick = INS_PER_TICK;
    int ticks_to_finish_first_task;
    long lateness_after_1_task;
    struct PBS_Task* first_task;
    struct PBS_Process* first_process;

    first_task = plan->cur_task;
    first_task->instructions_real = first_task->instructions_planned;
    first_process = plan->cur_process;

    // finish first task
    ticks_to_finish_first_task = (int) (plan->cur_task->instructions_real / (long) ins_per_tick) + 1;
    for( i = 0; i < ticks_to_finish_first_task; i++){
        pbs_handle_prediction_failure(plan);
    }
    lateness_after_1_task = first_task->instructions_real - first_task->instructions_planned;

    assert(plan->tasks_finished == 1);
    assert(plan->cur_task->task_id == 1);
}
// t1
void check_run_task_early_time(struct PBS_Plan * p) {

    struct PBS_Task* t1_addr = p->cur_task;
    long ticks_start = p->tick_counter;
    long ticks_end;
    long duration;
    long ticks_to_finish = p->cur_task->instructions_real / INS_PER_TICK;
    p->cur_task->instructions_real = p->cur_task->instructions_planned - 100;
    while (p->cur_task == t1_addr){
        pbs_handle_prediction_failure(p);
    }
    ticks_end = p->tick_counter;
    duration = ticks_end - ticks_start;
    assert(duration == ticks_to_finish + 1 || duration == ticks_to_finish);
}
// t2
void check_run_task_tm2_early_time(struct PBS_Plan *p){
    //todo: implement
    struct PBS_Task* t_2 = p->cur_task;
    assert(p->tasks_finished == 2);
    p->cur_task->instructions_real = ((p->cur_task->instructions_planned * (T2_SIGMA / 10) / 100) - 13430718);
    assert(p->cur_task->instructions_real < p->cur_task->instructions_planned);
    while (t_2->state != PLAN_TASK_FINISHED){
        assert(p->cur_task->task_id == 2);
        pbs_handle_prediction_failure(p);
    }
    assert(p->state == SIGNALED);

}
//t3
void check_run_task_late_time(struct PBS_Plan *p){
    struct PBS_Task* t3 = p->cur_task;
    assert(p->tasks_finished == 3);
    p->cur_task->instructions_real = p->cur_task->instructions_planned + 100;

    while (t3->state != PLAN_TASK_FINISHED){
        assert(p->cur_task->task_id == 3);
        pbs_handle_prediction_failure(p);
    }

}

//t4
void check_preempt_task(struct PBS_Plan *p){
    assert(p->tasks_finished == 4);
    struct PBS_Task* old_addr_t4 = p->cur_task;
    long old_slot_owner = old_addr_t4->slot_owner;
    long new_slot_owner;
    struct PBS_Task* new_addr_t4;
    long t4_id = p->cur_task->task_id;
    p->cur_task->instructions_real = p->cur_task->instructions_planned + PREEMPTION_LIMIT + 1;
    while(p->cur_task->task_id == t4_id){
        pbs_handle_prediction_failure(p);
    }

    new_addr_t4 = find_task_with_task_id(p, t4_id);
    new_slot_owner = new_addr_t4->slot_owner;
    assert(new_addr_t4 > old_addr_t4);
    assert(old_slot_owner != new_slot_owner);
}

void check_signal_t2_task(struct PBS_Plan *p){
    //todo: implement

}
/**
 * Check if Threshold calculation works
 * @param p
 */
void check_thresholds(struct PBS_Plan* p){
    short result;
    result = check_t1(p);
    result = check_t2_task(p);
    result = check_tm2_task(p);

    result = check_t2_process(p);
    result = check_tm2_process(p);

    result = check_t2_node(p);
    result = check_tm2_node(p);
}
/**
 * Tests if parsing of Plan works
 * @param plan
 */
void test_plan_parsing(struct PBS_Plan* plan){
    // --- read plan ---
    FILE *fp = fopen(PLAN_PATH, "r");
    long buffer_size = get_file_size(fp);
    char plan_string[buffer_size];
    read_plan(fp, plan_string, buffer_size);
    // --- parse plan ---
    parse_plan(plan_string, plan);

    struct PBS_Task* t_ptr = plan->tasks;
    assert(number_processes_in_plan(plan) == 3);
}

/**
 * reads the file into the buffer
 * @param fp file_descriptor for plan-text-file
 * @param buffer buffer that contain chars
 */
void read_plan(FILE* fp, char* buffer, long length_buffer){
    int len_buf_int = (int) length_buffer;
   fgets(buffer,len_buf_int, fp);
   buffer[length_buffer-1] = '\0';
   fclose(fp);

}


long get_file_size(FILE* fp){
    fseek(fp, 0, SEEK_END); // seek to end of file
    long size = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET); // seek back to beginning of file
    return size + 1; // to include '\0'
}

/**
 * Runs pbs_handle_prediction_failure method and updates pointer t if plan is changed
 * @param p
 * @param t
 */
struct PBS_Task * run(struct PBS_Plan* p, struct PBS_Task* t){
    long cur_task_id = t->task_id;
    pbs_handle_prediction_failure(p);
    if (cur_task_id != p->cur_task->task_id){
        return find_task_with_task_id(p, cur_task_id);
    } else {
        return t;
    }
}