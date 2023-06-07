import os

n_processes = [48, 480, 2112]
for n_process in n_processes:
    os.system("srun -n %d ./barrier-ring" % (n_process))
    os.system("srun -n %d ./barrier-tree" % (n_process))
