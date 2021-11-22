# Level Two PBS-Prediction-Failure Handling

This repository contains the level two implemantation for the prediction failure handling component as well as the program that transforms the level two code into kernel code as well as some other helper scripts and tests.

This repository also contains defintions in **pbs_entities.h** and **config.h** that are designed to be reused throughout the system. If other repsitories are cloned, the corresponding **config.h/pbs_entities.h** should be symlinked to the files in this directory.

## Run as userland program

1. Clone this repository
1. Change the macro `PLAN_PATH` in `main.c` to the absolute path of `test/plan.log`.
2. Run Cmake in the current directory `cmake .`
3. Run make `make`
4. Run the executable `./level2`
