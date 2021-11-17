
// TODO: follow commenting pattern from first t2 defintions
// --- ADMIN STUFF ---
#define LOG_PBS 1
#define MAX_NUMBER_PROCESSES 100
#define MAX_NUMBER_TASKS_IN_PLAN 400
#define MAX_LEN_PLAN_STR 20000
#define ACCURACY 10000

// --- GENERAL ---
#define PBS_HZ 100
#define INS_PER_SEC 1000000000
#define INS_PER_TICK (INS_PER_SEC/PBS_HZ)
#define RESCHEDULE_DELAY 30
#define RESCHEDULE_TIME (PBS_HZ*RESCHEDULE_DELAY)

// --- T1 ---
#define T1_MAX_TICKS_OFF 10
#define T1_MIN_TICKS_OFF 2
#define PREEMPTION_LIMIT T1_MAX_TICKS_OFF * INS_PER_TICK
#define T1_NO_PREEMPTION T1_MIN_TICKS_OFF * INS_PER_TICK
#define T1_SIGMA 125

// --- T2 ---
#define T2_SIGMA (T1_SIGMA + 50) // percentage as int; max allowed deviation % of a task from its plan
#define T2_SPACER (T1_MAX_TICKS_OFF * INS_PER_TICK ) // raw number instructions; Distance t1 -> t2_task
#define T2_TASK_SIGNALING_LIMIT  (PBS_HZ * INS_PER_TICK)// raw number instructions; t2_task max value TODO: Implement
#define T2_CAPACITY_BUFFER 110 // percentage as int, underestimation of node computational capacity
#define T2_ASSIGNABLE_PLAN_BUFFER 50 // Factor that describes what percentage of the buffer may be used up, e.g. 50 with a 1000 buffer means, that only a buffer of 500 may be used before a prediction failure will be send
#define MINIMUM_USABLE_BUFFER 5//buffer that is usable for very little proces progression
#define T2_MAX_PREEMPTIONS 5
#define T2_NODE_CHECK_ENABLED 0
#define FREE_TIME  10 // Factor that stores stretch happening in plan, e.g. free_time = 110, CPU is 110% faster then assumed by plan
#define STRESS_RESET (PBS_HZ*30)

// --- RESCHEDULING ---
#define STRETCH_CONSTANT 105 // percentage as int; determines how much tasks are stretched by rescheduling
#define SHRINK_CONSTANT 95 // percentage as int; determines how much tasks are shrunk by rescheduling

// --- enable/disable thresholds ---

#define T1_ENABLED 1
#define T2_TASK_ENABLED 1
#define TM2_TASK_ENABLED 1

#define T2_PROCESS_ENABLED 1
#define TM2_PROCESS_ENABLED 1

#define T2_NODE_ENABLED 1
#define TM2_NODE_ENABLED 1