import os

sizes = [100, 1000, 3000, 5000]
n_threads = [1, 2, 4, 8, 16, 24, 32, 48]
for size in sizes:
    for p in n_threads:
        os.system("srun -N 1 -n 1 -c 48 -t 0 ./lab2 %d %d" % (size, p))
