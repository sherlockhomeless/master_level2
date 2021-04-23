//
// Created by ml on 23.03.21.
//
/**
 * todo: nice_to_have_
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "plan/plan.h"
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
void test_insert_preempted_tasks(); //todo: imp
void test_task_moving();
int test_run();

struct PBS_Task * run(struct PBS_Plan *p, struct PBS_Task* t);


int main(){
    run_unit_tests();
    return test_run();

}





int test_run(){
    PredictionFailureSignal*  sig;
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
        schedule(plan_ptr);
    }

    for (int i = 0; i < 3; i++){
        sig = get_signal(i);
        printf("signal: tick=%ld type=%d, task=%ld, process=%ld\n",
               sig->tick, sig->type_signal, sig->cur_task_id, sig->cur_process_id);
    }
    return 0;
}
// t0
void check_run_task_on_time(struct PBS_Plan* plan){
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
    for(int i = 0; i < ticks_to_finish_first_task; i++){
        schedule(plan);
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
        schedule(p);
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
    p->cur_task->instructions_real = ((p->cur_task->instructions_planned * (CAP_LATENESS/10)/100) - 13430718);
    assert(p->cur_task->instructions_real < p->cur_task->instructions_planned);
    while (t_2->state != PLAN_TASK_FINISHED){
        assert(p->cur_task->task_id == 2);
        schedule(p);
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
        schedule(p);
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
        schedule(p);
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
void test_plan_parsing(struct PBS_Plan* plan){
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
    struct PBS_Task* t_ptr = plan->tasks;
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
struct PBS_Task * run(struct PBS_Plan* p, struct PBS_Task* t){
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
    struct PBS_Plan* plan = (struct PBS_Plan*) malloc(sizeof(struct PBS_Plan));
    test_plan_parsing(plan);

    plan->tasks[0].slot_owner = plan->tasks[1].task_id;
    plan->tasks[1].process_id = 0;
    preempt_cur_task(plan);
}

void test_find_slot_to_move_to(){
    struct PBS_Plan p = {0};
    fill_empty_test_plan(&p);
    struct PBS_Process processes [MAX_NUMBER_PROCESSES];

    long order[5] = {0,1,2,3,0};
    long index;

    for (int i = 0; i < 5; i++){
        p.tasks[i].process_id = order[i];
    }

    index = find_slot_to_move_to(0, &p );
    assert(index == 3);
}

void test_move_others(){
    struct PBS_Plan p = {0};
    fill_empty_test_plan(&p);
    struct PBS_Task* tasks = &p.tasks[0];
    long order[5] = {0,1,2,3,4};
    for(int i = 0; i < 5; i++){
        tasks[i].process_id = order [i];
        tasks[i].task_id = order[i];
        tasks[i].slot_owner = SHARES_NO_SLOT;
    }

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
    struct PBS_Plan p = {0};
    fill_empty_test_plan(&p);
    struct PBS_Task* tasks = &p.tasks[0];

    long order[5] = {0,1,2,3,4};
    for(int i = 0; i < 5; i++){
        tasks[i].process_id = order [i];
        tasks[i].task_id = order[i];
    }

    // 0-1-2-3-4 > 0-1-2-0-4
    move_preempted_tasks(3,1,tasks, &p);

    assert(tasks[3].task_id == 0);
    assert(tasks[3].slot_owner == 4);
    assert(tasks[4].task_id == 4);
    assert(tasks[2].task_id == 2);
}
