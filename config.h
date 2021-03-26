//
// Created by ml on 26.03.21.
//

#ifndef LEVEL2_THRESHOLD_CONFIG_H
#define LEVEL2_THRESHOLD_CONFIG_H

#endif //LEVEL2_THRESHOLD_CONFIG_H

// --- ADMIN STUFF ---
#define LOG 1


// --- GENERAL ---
#define INS_PER_SEC 4000000000
#define HZ 250
#define INS_PER_TICK INS_PER_SEC/HZ

// --- T1 ---
#define MAX_TICKS_OFF 20
#define MIN_TICKS_OFF 2
#define PREEMPTION_LIMIT MAX_TICKS_OFF * INS_PER_TICK
#define NO_PREEMPTION MIN_TICKS_OFF * INS_PER_TICK
#define SIGMA_T1 2.0

// --- T2 ---
#define CAP_LATENESS 1.5
#define ASSIGNABLE_BUFFER 0.5
#define MINIMUM_USABLE_BUFFER 0.05 //buffer that is usable for very little proces progression
#define T2_SPACER 2.0 // Times t2 needs to be bigger then t1
#define T2_MAX_PREEMPTIONS 5
#define T2_NODE_CHECK_ENABLED 0





