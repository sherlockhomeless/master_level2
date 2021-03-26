cmake_minimum_required(VERSION 3.17)
project(level2 C)

set(CMAKE_C_STANDARD 99)

add_executable(level2 plan/plan.c plan/plan.h process.h task.h threshold_checking.h track.h plan/parse_plan.c main.c process.c task.c pb-scheduler.c pb-scheduler.h pmu_interface.c pmu_interface.h threshold_checking.c config.h prediction_failure_handling.c prediction_failure_handling.h)