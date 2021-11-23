# Level Two PBS-Prediction-Failure Handling

This repository contains the level two implemantation for the prediction failure handling component as well as the program that transforms the level two code into kernel code as well as some other helper scripts and tests.

This repository also contains defintions in **pbs_entities.h** and **config.h** that are designed to be reused throughout the system. If other repsitories are cloned, the corresponding **config.h/pbs_entities.h** should be symlinked to the files in this directory.

## Run as userland program

1. Clone this repository
1. Change the macro `PLAN_PATH` in `main.c` to the absolute path of `test/plan.log`.
2. Run Cmake in the current directory `cmake .`
3. Run make `make`
4. Run the executable `./level2`

## Known Bugs

* Running the level2 on userland would sometimes cause a segmentation fault when running on a VM. The bug seems to be solved now, but nothing was actually done to mitigate it. When looking at the behaviour with gdb, it seemed that in `test_reschedule()` the value of `p->cur_process` was pointing to another address than `&p->process[0]` which it is set to. When running it on the host system, the pointer had a reasonable address. Testing this bug on two machines in both cases, the VMs had issues and running it on the host seemed to do the trick, so if you encounter this segfault issue, maybe try compiling/running the program without an supervisor.