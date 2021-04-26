#include <stdio.h>
#include <stdlib.h>
#include "plan.h"
#include "prediction_failure_config.h"
#include "pbs_entities.h"


// defining symbols for parsing
#define NUMBER 0b0
#define COMMA 0b1
#define SEMI 0b10
#define META_END 0b11
#define END 0b1111


char* parse_meta(char* plan_s, char* cur_position, struct PBS_Plan* p);
void parse_tasks(char* plan_s, char* cur_position, struct PBS_Plan* p);

void parse_next_process(char** str, struct PBS_Process* process_information);
char* parse_next_task(struct PBS_Plan * , int , char* );
char parse_cur_symbol(char *str);
long parse_next_number(char** str_ptr);
long count_tasks(char* task_list);

struct PBS_Plan* parse_plan(char* plan_s, struct PBS_Plan* plan){
    char* cur_position = plan_s;
    // parse meta-section until we find ';;;'
    cur_position = parse_meta(plan_s, cur_position, plan);
    parse_tasks(plan_s, cur_position, plan);
    change_plan_state(plan, ON_PLAN);
    plan->index_cur_task = 0;
    plan->tasks_finished = 0;
    plan->cur_task = plan->tasks;
    plan->finished_tasks = plan->tasks;
    plan->cur_process = &plan->processes[plan->cur_task->process_id];
    plan->tick_counter = 0;
    return plan;
}


/*
 * @brief parses the meta information contained in the p
 * @param plan_s: String of the p to be parsed
 * @param cur_position: Pointer to the current position in the parsing process
 * @param p: Target data structure to insert data into
 */
char* parse_meta(char* plan_s, char* cur_position, struct PBS_Plan* p){
    /* FLAGS:
    0b0 => found nothing
    0b1 => found number of processes
    */
    char cur_symbol;
    char state = 0b0;
    char found_end = 0b0;
    int process_counter = 0;

    // indices: process_id, buffer
    long process_information [2];

    while(!found_end) {
        cur_symbol = parse_cur_symbol(cur_position);

        // parse number of processes
        if (cur_symbol == NUMBER && state == 0b0) {
            long number_processes = parse_next_number(&cur_position);
            p->num_processes = number_processes;
            struct PBS_Process *ptr_process_information = (struct PBS_Process *) malloc(number_processes * sizeof( struct PBS_Process));
            //p->processes[0] = ptr_process_information;

            p->num_processes = number_processes;
            state = 0b1;
            cur_position++;

        } else if (cur_symbol == SEMI && parse_cur_symbol(cur_position + 1) == SEMI) {
            cur_position += 2;
            found_end = 1;
            continue;
        } else {
            p->processes[process_counter].num_tasks_remaining = 0;
            p->processes[process_counter].lateness = 0;
            p->processes[process_counter].length_plan = 0;
            p->processes[process_counter].instructions_retired = 0;
            p->processes[process_counter].process_id = 0;
            p->processes[process_counter].buffer = 0;


            parse_next_process(&cur_position, &p->processes[process_counter]);
            process_counter++;
        }
    }
    // setup process for unallocated time slots
    p->processes[MAX_NUMBER_PROCESSES-1].process_id = -1;
    p->processes[MAX_NUMBER_PROCESSES-1].buffer = -1;
    p->processes[MAX_NUMBER_PROCESSES-1].length_plan = 0;
    return cur_position;
}

void parse_tasks(char* plan_s, char* cur_position, struct PBS_Plan* p) {
    int i;
    long length_all_tasks;
    p->num_tasks = count_tasks(cur_position);
    length_all_tasks = p->num_tasks + 1;

    for ( i = 0; i < p->num_tasks; i++){
        cur_position = parse_next_task(p, i, cur_position);
        if(LOG_PBS && 0)
            printf("task %ld: pid=%ld, length_plan=%ld @=%p\n ", p->tasks[i].task_id, p->tasks[i].process_id, p->tasks[i].instructions_planned, &p->tasks[i]);
    }
    // signals end of plan
    p->tasks[i].task_id = -2;
    p->tasks[i].process_id = -2;
}

/**
 * Reads a task from a string into the task list on index at the given plan
 * @param plan
 * @param index
 * @param cur_position: Pointing to the first character in the tasks process
 * @return
 */
char* parse_next_task(struct PBS_Plan* plan, int index, char* cur_position){
    struct PBS_Task cur_t;
    cur_t.lateness = 0;
    cur_t.instructions_retired_slot = 0;
    cur_t.instructions_retired_task = 0;
    cur_t.state = PLAN_TASK_WAITING;
    cur_t.slot_owner = SHARES_NO_SLOT;

    cur_t.process_id = parse_next_number(&cur_position);
    plan->processes[cur_t.process_id].num_tasks_remaining++;
    cur_position++;
    cur_t.task_id = parse_next_number(&cur_position);
    cur_position++;
    cur_t.instructions_planned = parse_next_number(&cur_position);
    plan->processes[cur_t.process_id].length_plan += cur_t.instructions_planned;
    cur_position++;
    cur_t.instructions_real = parse_next_number(&cur_position);
    cur_position++;
    plan->tasks[index] = cur_t;


    return cur_position;
}
/*
 * Counts the task in the plan-string by counting semicolons
 */
long count_tasks(char* task_list_s){
    long counter = 0;
    while(*task_list_s != '\0'){
        if(*task_list_s == ';'){
            counter++;
        }
        task_list_s++;
    }
    return counter;
}
char parse_cur_symbol(char* str){
    switch(*str){
        case ',':
            return COMMA;
        case ';':
            return SEMI;
        case '\n':
            return END;
        default:
            return NUMBER;
    }
}

// ! pointer at first char after last number
void parse_next_process(char** str,struct PBS_Process* process_information){
    process_information->process_id = parse_next_number(str);
      *str+=1;
    process_information->buffer = parse_next_number(str);
      *str+=1;
}

// ! pointer at last digit of number
long parse_next_number(char **str_ptr) {
    char *start = *str_ptr;
    // reads job_id
    while (parse_cur_symbol(*str_ptr) == NUMBER) {
        *str_ptr = *str_ptr + 1;
    }
    long res = strtoll(start, str_ptr, 10);
    return res;
}
