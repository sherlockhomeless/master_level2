#!/usr/bin/bash

helper/make_src_kernel_ready.py . /home/ml/hdd/kelvin/kernel/sched/prediction_failure_handling
rsync -rhI --info=progress2 /home/ml/hdd/kelvin/kernel/sched/ vagrant@pbs-vm:/home/vagrant/kernel_src/kelvin/kernel/sched/

