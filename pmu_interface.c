//
// Created by ml on 26.03.21.
//
#include <stdio.h>
#include "kernel_dummies.h"

#include "pmu_interface.h"
#include "config.h"

void test_if_working(void);

long get_retired_instructions(){
    return INS_PER_TICK;
}
EXPORT_SYMBOL(get_retired_instructions);

void test_if_working(){
    int i = 0;
    printf("testing %d", i);
}
EXPORT_SYMBOL(test_if_working);