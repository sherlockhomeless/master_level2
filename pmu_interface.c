//
// Created by ml on 26.03.21.
//
#include "kernel_dummies.h"

#include "pmu_interface.h"
#include "prediction_failure_config.h"

long get_retired_instructions(){
    return INS_PER_TICK;
}