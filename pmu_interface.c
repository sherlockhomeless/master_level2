//
// Created by ml on 26.03.21.
//
#include <stdio.h>
#include "kernel_dummies.h"

#include "pmu_interface.h"
#include "config.h"

long get_retired_instructions(){
    return INS_PER_TICK;
}
EXPORT_SYMBOL(get_retired_instructions);