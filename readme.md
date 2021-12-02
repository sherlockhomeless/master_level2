# Level Two PBS-Prediction-Failure Handling

This repository contains the level two implemantation for the prediction failure handling component as well as the program that transforms the level two code into kernel code as well as some other helper scripts and tests.

This repository also contains defintions in **pbs_entities.h** and **config.h** that are designed to be reused throughout the system. If other repsitories are cloned, the corresponding **config.h/pbs_entities.h** should be symlinked to the files in this directory.

## Run as userland program

1. Clone repository
1. Change the macro `PLAN_PATH` in `main.c` to the absolute path of `test/plan.log`.
2. Run Cmake in the current directory `cmake .`
3. Run make `make`
4. Run the executable `./level2`

## Convert Userland => Kernel

1. Clone fork of master-thesis to extrace changed source-files into: `git clone https://github.com/sherlockhomeless/master_thesis_linux.git -b pb --single-branch`
1. Conversion is done via `make_src_kernel_ready.py`. The script is based on python 3.8. It takes 2 parameters:
    1. **$1** points to the path where the level2 source files are located
    2. **$2** points to where the output should be written to. This should be $BASE_PATH_STEP1/kernel/sched/prediction_failure_handling; make sure that the path exists before running the script
2. Check the generated Makefile present at **$2** so that no files are accidentally present that are not supposed to be there due to the compilation for example
3. Compile & run kernel as usually


## Known Bugs

* Running the level2 on userland would sometimes cause a segmentation fault when running on a VM. The bug seems to be solved now, but nothing was actually done to mitigate it. When looking at the behaviour with gdb, it seemed that in `test_reschedule()` the value of `p->cur_process` was pointing to another address than `&p->process[0]` which it is set to. When running it on the host system, the pointer had a reasonable address. Testing this bug on two machines in both cases, the VMs had issues and running it on the host seemed to do the trick, so if you encounter this segfault issue, maybe try compiling/running the program without an supervisor.
