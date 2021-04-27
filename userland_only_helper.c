//
// Created by ml on 27/04/2021.
//

#include <stdio.h>

#include "pbs_entities.h"
#include "userland_only_helper.h"

void write_binary_to_file(struct PBS_Plan* plan, char* binary_path){
    FILE *fp;
    fp = fopen(binary_path, "w");

    if (fp == NULL){
        printf("error opening file\n");
        return;
    }

    //https://www.tutorialspoint.com/c_standard_library/c_function_fwrite.htm
    fwrite(plan, sizeof(struct PBS_Plan),1, fp);
}

void show_tasks(struct PBS_Plan* p){
    int i;
    struct PBS_Task task_list[MAX_NUMBER_TASKS_IN_PLAN];

    for ( i = 0; i < p->num_tasks; i++){
        task_list[i] = *(p->tasks + i);
    }
    printf("[SHOW_TASKS]%ld:", p->tick_counter);
    for ( i = 0; i < p->num_tasks; i++){
        printf("%d:(%ld, %ld) ", i, task_list[i].task_id, task_list[i].process_id);
    }
    printf("!\n");
}