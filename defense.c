#include "plan.h"
#include "pbs_entities.h"
#include "task.h"
#include "defense.h"
#include "pb-scheduler.h"
#include "assert.h"
#include "prediction_failure_signaling.h"

void run_defense() {
    show_t1_preemption();
    show_t2_node_signal();
    show_tm2_task_signal();
}
void show_t1_preemption(){
    /*
     * TASKS (task-id/pid/preempted):
     * (0,0,no), (1,1,yes), (2,0,no), (3,1,no)
     * expected after state after running:
     * (0,0,no), (2,0,no),(1,1,yes), (3,1,no)
     */
    struct PBS_Plan plan = {0};
    struct PBS_Plan* plan_ptr = &plan;
    setup_plan(plan_ptr);

    // arguments Task-id, pid, instructions planned, instructions real
    struct PBS_Task t0 = create_task(0, 0, INS_PER_TICK, INS_PER_TICK);
    // max ticks off is 10, PREEMPTION LIMIT captures this notion
    struct PBS_Task t1 = create_task(1,1, INS_PER_TICK, PREEMPTION_LIMIT+1);
    struct PBS_Task t2 = create_task(2,0, INS_PER_TICK, INS_PER_TICK);
    struct PBS_Task t3 = create_task(3,1, INS_PER_TICK, INS_PER_TICK);
    struct PBS_Task end = create_task(-2,-2, 0, 0);

    plan.tasks[0] = t0;
    plan.tasks[1] = t1;
    plan.tasks[2] = t2;
    plan.tasks[3] = t3;
    plan.tasks[4] = end;

    pbs_run_timer_tick(plan_ptr);
    // expectation t0 has finished
    assert(plan.cur_task->task_id == 1);
    // run until current task-id != 1
    while (plan.cur_task->task_id == 1){
        pbs_run_timer_tick(plan_ptr);
    }
    // expectation: order of tasks: 0,2,1,3
    assert(plan.tasks[0].task_id == 0);
    assert(plan.tasks[1].task_id == 2);
    assert(plan.tasks[2].task_id == 1);
    assert(plan.tasks[3].task_id == 3);
    print_plan_state(plan_ptr, 0, 3);
}

void show_t2_node_signal(){
    /*
     * setup:
     * bunch of tasks, that each take 20% longer than planned
     * expected:
     * after some amount of ticks, tm2_node signal will be given since it node lateness is capped at 10%
     */
    int i;
    long ins_planned = 0;
    struct PBS_Plan plan = {0};
    struct PBS_Plan* plan_ptr = &plan;
    setup_plan(plan_ptr);
    plan_ptr->num_processes = 2;

    for (i = 0; i < 200; i++){
        plan.tasks[i] = create_task(i, i%2, INS_PER_SEC, (INS_PER_SEC/100)*120);
        ins_planned += INS_PER_SEC;
    }
    plan.tasks[200] = create_task(-2, -2, 0, 0);
    plan.instructions_planned = ins_planned;

    while (plan_ptr->tick_counter < 11528) {
        pbs_run_timer_tick(plan_ptr);
    }
    // this timer tick will cause a t2-node signal
    pbs_run_timer_tick(plan_ptr);
    // lateness will be reset & a reschedule will have happened
    pbs_run_timer_tick(plan_ptr);
}

void show_tm2_task_signal() {
   /*
    * setup:
    * a task which takes 10% of its planned length is run
    * expectation:
    * task will trigger tm2_task
    */

    struct PBS_Plan plan = {0};
    struct PBS_Plan* plan_ptr = &plan;
    setup_plan(plan_ptr);

    struct PBS_Task t0 = create_task(0, 0, INS_PER_SEC, INS_PER_SEC/10);
    struct PBS_Task t1 = create_task(1,1, INS_PER_TICK, PREEMPTION_LIMIT+1);
    struct PBS_Task end = create_task(-2,-2, 0, 0);

    plan.tasks[0] = t0;
    plan.tasks[1] = t1;
    plan.tasks[2] = end;

    while (plan_ptr->cur_task->task_id == 0) {
        pbs_run_timer_tick(plan_ptr);
    }

    print_plan_state(plan_ptr, 0,2);

}

