from_vm:
	rsync -rv vagrant@pbs-vm:kernel_src/master_thesis_linux /home/ml/hdd/rsync_vm/
to_vm:
	rsync -rv --ignore-times /home/ml/hdd/rsync_vm/master_thesis_linux/kernel/sched vagrant@pbs-vm:/home/vagrant/kernel_src/master_thesis_linux/kernel
level2:
	cd .. && helper/make_src_kernel_ready.py /home/ml/Dropbox/Master-Arbeit/code/level2/ /home/ml/hdd/rsync_vm/master_thesis_linux/
