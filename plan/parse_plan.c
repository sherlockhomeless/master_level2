#include "plan.h"
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>

// defining symbols for parsing
#define NUMBER 0b0
#define COMMA 0b1
#define SEMI 0b10
#define META_END 0b11
#define END 0b1111


char* parse_meta(char* plan_s, char* cur_position, Plan* plan);
void parse_tasks(char* plan_s, char* cur_position, Plan* plan);

char parse_cur_symbol(char *str);
void parse_next_process(char** str, PlanProcess* process_information);
long parse_next_number(char** str_ptr);
long count_tasks(char* task_list);
char* read_task(Plan* , int , char* );

Plan* parse_plan(char* plan_s, Plan* plan){

    char* cur_position = plan_s;
    // parse meta-section until we find ';;;'
    cur_position = parse_meta(plan_s, cur_position, plan);
    parse_tasks(plan_s, cur_position, plan);
    return plan;
}


/*
 * @brief parses the meta information contained in the plan
 * @param plan_s: String of the plan to be parsed
 * @param cur_position: Pointer to the current position in the parsing process
 * @param plan: Target data structure to insert data into
 */
char* parse_meta(char* plan_s, char* cur_position, Plan* plan){
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
            plan->num_processes = number_processes;
            PlanProcess *ptr_process_information = (PlanProcess *) malloc(number_processes * sizeof(PlanProcess));
            plan->processes = ptr_process_information;
            state = 0b1;
            cur_position++;

            if (LOG)
                printf("number of processes found: %ld\n", plan->num_processes);

        } else if (cur_symbol == SEMI && parse_cur_symbol(cur_position + 1) == SEMI) {
            cur_position += 2;
            found_end = 1;
            continue;
        } else {
            plan->processes[process_counter].num_tasks = 0;
            plan->processes[process_counter].lateness = 0;
            plan->processes[process_counter].length_plan = 0;
            plan->processes[process_counter].instructions_retired = 0;
            plan->processes[process_counter].process_id = 0;
            plan->processes[process_counter].buffer = 0;


            parse_next_process(&cur_position, &plan->processes[process_counter]);

            if(LOG)
                printf("process %ld: buffer=%ld\n", plan->processes[process_counter].process_id, plan->processes[process_counter].buffer);
            process_counter++;
        }
    }
    return cur_position;
}

void parse_tasks(char* plan_s, char* cur_position, Plan* plan) {
    plan->num_tasks = count_tasks(cur_position);
    plan->tasks = (Task*) malloc(plan->num_tasks * sizeof (Task));

    for (int i = 0; i < plan->num_tasks; i++){
        cur_position = read_task(plan, i, cur_position);
        if(LOG)
            printf("task %ld: pid=%ld, length_plan=%ld @=%p\n ", plan->tasks[i].task_id, plan->tasks[i].process_id, plan->tasks[i].instructions_planned,  &plan->tasks[i]);
    }


}
/**
 * Reads a task from a string into the task list on index at the given plan
 * @param plan
 * @param index
 * @param cur_position: Pointing to the first character in the tasks process
 * @return
 */
char* read_task(Plan* plan, int index, char* cur_position){
    Task cur_t;
    cur_t.lateness = 0;
    cur_t.instructions_retired = 0;

    cur_t.process_id = parse_next_number(&cur_position);
    cur_position++;
    cur_t.task_id = parse_next_number(&cur_position);
    cur_position++;
    cur_t.instructions_planned = parse_next_number(&cur_position);
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
void parse_next_process(char** str, PlanProcess* process_information){
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