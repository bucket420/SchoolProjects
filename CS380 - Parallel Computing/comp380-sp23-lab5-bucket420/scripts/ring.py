import os

sizes = [1, 8, 64, 512, 4096, 32768]
n_processes = [2, 4, 8, 16, 32]
for size in sizes:
    for n_process in n_processes:
        os.system("srun -n %d -N %d ./ring %d" % (n_process, n_process, size))
