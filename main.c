//
// Created by ml on 23.03.21.
//

#include <stdio.h>
#include <stdlib.h>
#include "plan/plan.h"
#include "pb-scheduler.h"

char* PLAN_PATH = "/home/ml/Dropbox/Master-Arbeit/code/level2/test/plan.log";
void read_plan(FILE*, char* , long);
long get_file_size(FILE*);
void test_plan_parsing(Plan*);


int main(){
    Plan* plan = (Plan*) malloc(sizeof(Plan));
    test_plan_parsing(plan);
    printf(" first task ins_planned should be 172361973, is %ld\n", plan->tasks[0].instructions_planned);
    printf(" last task ins_planned should be 426731023, is %ld\n", plan->tasks[316].instructions_planned);
    schedule(plan);

    return 0;
}

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
}

/**
 * reads the file into the buffer
 * @param fp filedescriptor for plan-text-file
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


