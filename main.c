//
// Created by ml on 23.03.21.
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "plan/plan.h"
#include "pb-scheduler.h"
#include "threshold_checking.h"
#include "config.h"
#include "prediction_failure_handling.h"

char* PLAN_PATH = "/home/ml/Dropbox/Master-Arbeit/code/level2/test/plan.log";

void read_plan(FILE*, char* , long);
long get_file_size(FILE*);
void test_plan_parsing(Plan*);
void check_thresholds(Plan*);
void check_run_task_on_time(Plan *plan);
void check_run_task_early_time(Plan *plan);
void check_run_task_tm2_early_time(Plan *plan);
void check_run_task_late_time(Plan *plan);
void check_preempt_task(Plan *plan);
void check_signal_t2_task(Plan *plan);

void run_unit_tests();
void test_find_slot_to_move_to();
void test_move_others();
void test_insert_preempted_tasks(); //todo: imp
void test_task_moving();
int test_run();
Task * run(Plan *p, Task * t);


int main(){
    run_unit_tests();
    return test_run();

}

int test_run(){
    Task* cur_task;
    Plan* plan = (Plan*) malloc(sizeof(Plan));
    test_plan_parsing(plan);
    check_thresholds(plan);
    check_run_task_on_time(plan); // finish t0
    check_run_task_early_time(plan); // finish t1
    check_run_task_tm2_early_time(plan); // finish t2
    check_run_task_late_time(plan);
    check_preempt_task(plan);
    while(plan->state != PLAN_FINISHED)
        schedule(plan);
    return 0;
}
// t0
void check_run_task_on_time(Plan* plan){
    long ins_per_tick = INS_PER_TICK;
    int ticks_to_finish_first_task;
    long lateness_after_1_task;
    Task* first_task;
    PlanProcess* first_process;

    first_task = plan->cur_task;
    first_task->instructions_real = first_task->instructions_planned;
    first_process = plan->cur_process;

    // finish first task
    ticks_to_finish_first_task = (int) (plan->cur_task->instructions_real / (long) ins_per_tick) + 1;
    for(int i = 0; i < ticks_to_finish_first_task; i++){
        schedule(plan);
    }
    lateness_after_1_task = first_task->instructions_real - first_task->instructions_planned;

    printf("process lateness should be %ld, is %ld\n", lateness_after_1_task, first_process->lateness);
    printf("node lateness should be %ld, is %ld\n", lateness_after_1_task, plan->lateness);

}
// t1
void check_run_task_early_time(Plan * p) {
    assert(p->tasks_finished == 1);
    assert(p->tasks->task_id == 1);
    assert(p->tasks == p->cur_task);

    Task* t1_addr = p->cur_task;
    long ticks_start = p->tick_counter;
    long ticks_end;
    long duration;
    long ticks_to_finish = p->cur_task->instructions_real / INS_PER_TICK;
    p->cur_task->instructions_real = p->cur_task->instructions_planned - 100;
    while (p->cur_task == t1_addr){
        schedule(p);
    }
    ticks_end = p->tick_counter;
    duration = ticks_end - ticks_start;
    assert(duration == ticks_to_finish + 1 || duration == ticks_to_finish);
}
// t2
void check_run_task_tm2_early_time(Plan *p){
    //todo: implement
    Task* t_2 = p->cur_task;
    assert(p->tasks_finished == 2);
    p->cur_task->instructions_real = ((p->cur_task->instructions_planned * (CAP_LATENESS/10)/100) - 13430718);
    assert(p->cur_task->instructions_real < p->cur_task->instructions_planned);
    while (t_2->state != TASK_FINISHED){
        assert(p->cur_task->task_id == 2);
        schedule(p);
    }
    assert(p->state == SIGNALED);

}
//t3
void check_run_task_late_time(Plan *p){
    Task* t3 = p->cur_task;
    assert(p->tasks_finished == 3);
    p->cur_task->instructions_real = p->cur_task->instructions_planned + 100;

    while (t3->state != TASK_FINISHED){
        assert(p->cur_task->task_id == 3);
        schedule(p);
    }

}

//t4
void check_preempt_task(Plan *p){
    assert(p->tasks_finished == 4);
    Task* old_addr_t4 = p->cur_task;
    long old_slot_owner = old_addr_t4->slot_owner;
    long new_slot_owner;
    Task* new_addr_t4;
    long t4_id = p->cur_task->task_id;
    p->cur_task->instructions_real = p->cur_task->instructions_planned + PREEMPTION_LIMIT + 1;
    while(p->cur_task->task_id == t4_id){
        schedule(p);
    }

    new_addr_t4 = find_task_with_task_id(p, t4_id);
    new_slot_owner = new_addr_t4->slot_owner;
    assert(new_addr_t4 > old_addr_t4);
    assert(old_slot_owner != new_slot_owner);
}

void check_signal_t2_task(Plan *p){
    //todo: implement

}
/**
 * Check if Threshold calculation works
 * @param p
 */
void check_thresholds(Plan* p){
    short result;
    result = check_t1(p);
    result = check_t2_task(p->cur_task, p);
    result = check_tm2_task(p->cur_task);

    result = check_t2_process(p->cur_process, 0);
    result = check_tm2_process(p->cur_process);

    result = check_t2_node(p);
    result = check_tm2_node(p);
}
/**
 * Tests if parsing of Plan works
 * @param plan
 */
void test_plan_parsing(Plan* plan){
    // --- read plan ---
    FILE *fp = fopen(PLAN_PATH, "r");
    long buffer_size = get_file_size(fp);
    char plan_string[buffer_size];
    read_plan(fp, plan_string, buffer_size);
    // --- parse plan ---
    parse_plan(plan_string, plan);

    printf("plan (@ %p) should have 317 tasks, has %ld, last task id should be 288, is %ld \n", plan,
           plan->num_tasks, plan->tasks[plan->num_tasks-1].task_id);

    // print whole parsed plan
    Task* t_ptr = plan->tasks;
    printf("[PLAN]");
    while(t_ptr->task_id != -2){
        printf("[%ld]-", t_ptr->task_id);
        t_ptr++;
    }
    printf("[%ld]", t_ptr->task_id);
    printf("\n");
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
 * Runs schedule method and updates pointer t if plan is changed
 * @param p
 * @param t
 */
Task * run(Plan* p, Task * t){
    long cur_task_id = t->task_id;
    schedule(p);
    if (cur_task_id != p->cur_task->task_id){
        return find_task_with_task_id(p, cur_task_id);
    } else {
        return t;
    }
}

void run_unit_tests(){
    test_find_slot_to_move_to();
    test_move_others();
    test_task_moving();
    test_insert_preempted_tasks();
}

void test_task_moving(){
    long first_task_id;
    long second_task_id;
    Plan* plan = (Plan*) malloc(sizeof(Plan));
    test_plan_parsing(plan);

    plan->tasks[0].slot_owner = plan->tasks[1].task_id;
    plan->tasks[1].process_id = 0;
    preempt_cur_task(plan);
}

void test_find_slot_to_move_to(){
    Plan p;
    Task* tasks = (Task*) malloc(sizeof (Task) * 5);
    PlanProcess processes [MAX_NUMBER_PROCESSES];

    long order[5] = {0,1,2,3,0};
    p.tasks = tasks;
    long index;

    for (int i = 0; i < 5; i++){
        tasks[i].process_id = order[i];
    }

    generate_test_plan(&p, processes, tasks);

    index = find_slot_to_move_to(0, &p );
    assert(index == 3);
}

void test_move_others(){
    Plan p;
    Task* tasks = (Task*) malloc(sizeof (Task) * 5);
    PlanProcess processes [MAX_NUMBER_PROCESSES];
    long order[5] = {0,1,2,3,4};
    for(int i = 0; i < 5; i++){
        tasks[i].process_id = order [i];
        tasks[i].task_id = order[i];
        tasks[i].slot_owner = SHARES_NO_SLOT;
    }
    generate_test_plan(&p, processes, tasks);

    move_other_tasks_forward(4, 1, &p);
    assert(tasks[0].task_id == 1);
    assert(tasks[3].task_id == 4);


    for (int i = 0; i < 5; i++){
        tasks[i].task_id = order[i];
    }

    move_other_tasks_forward(5, 2, &p);
    assert(tasks[0].task_id == 2);
    assert(tasks[1].task_id == 3);
}

void test_insert_preempted_tasks() {
    Plan p;
    Task* tasks = (Task*) malloc(sizeof (Task) * 5);
    PlanProcess processes [MAX_NUMBER_PROCESSES];

    long order[5] = {0,1,2,3,4};
    for(int i = 0; i < 5; i++){
        tasks[i].process_id = order [i];
        tasks[i].task_id = order[i];
    }


    generate_test_plan(&p, processes, tasks);
    // 0-1-2-3-4 > 0-1-2-0-4
    move_preempted_tasks(3,1,tasks, &p);

    assert(tasks[3].task_id == 0);
    assert(tasks[3].slot_owner == 4);
    assert(tasks[4].task_id == 4);
    assert(tasks[2].task_id == 2);
}
