//
// Created by ml on 23.03.21.
//

#include <stdio.h>
#include <stdlib.h>
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
void check_running_first_task(Plan*);
void test_task_moving();
int test_run();
Task * run(Plan *p, Task * t);


int main(){
    test_task_moving();
    return test_run();

}

int test_run(){
    Task* cur_task;
    Plan* plan = (Plan*) malloc(sizeof(Plan));
    test_plan_parsing(plan);
    check_thresholds(plan);

    check_running_first_task(plan);

    // set task 2 so it will be preempted
    // fixme: After moving around in plan -> cur_task changes too
    cur_task = plan->cur_task;
    cur_task->instructions_real = cur_task->instructions_planned * SIGMA_T1 + 1;
    while(cur_task->state != TASK_PREEMPTED){
        cur_task = run(plan, cur_task);
    }
    printf("[MAIN] Preemption worked");

    long task_order[] = {2,1,3,4};
    plan->tasks->instructions_real = plan->tasks->instructions_planned;
    plan->tasks[2].instructions_real =  plan->tasks[2].instructions_planned;
    plan->tasks[3].instructions_real =  plan->tasks[3].instructions_planned;

    printf("[MAIN] checking right order of execution for interrupted task");

    // fix does not stop here
    while (plan->cur_task->task_id == 2 || plan->cur_task->task_id == 1 || plan->cur_task->task_id == 3 || plan->cur_task->task_id == 4 ){
        if (plan->tick_counter==188){
            printf("del");
        }
        run(plan, cur_task);
        show_tasks(plan);
    }
    return 0;
    if(plan->state == PLAN_FINISHED)
        return 0;
    else
        return 1;
}

void check_running_first_task(Plan* plan){
    long ins_per_tick = INS_PER_TICK;
    int ticks_to_finish_first_task;
    long lateness_after_1_task;
    Task* first_task;
    PlanProcess* first_process;

    first_task = plan->cur_task;
    first_task->instructions_real = first_task->instructions_planned + 100;
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

/**
 * Check if Threshold calculation works
 * @param plan
 */
void check_thresholds(Plan* plan){
    short result;
    result = check_t1(plan);
    result = check_t2_task(plan->cur_task);
    result = check_tm2_task(plan->cur_task);

    result = check_t2_process(plan->cur_process, 0);
    result = check_tm2_process(plan->cur_process);

    result = check_t2_node(plan);
    result = check_tm2_node(plan);
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

void test_task_moving(){
    long first_task_id;
    long second_task_id;
    Plan* plan = (Plan*) malloc(sizeof(Plan));
    test_plan_parsing(plan);

    plan->tasks[0].slot_owner = plan->tasks[1].task_id;
    plan->tasks[1].process_id = 0;
    preempt_cur_task(plan);
}