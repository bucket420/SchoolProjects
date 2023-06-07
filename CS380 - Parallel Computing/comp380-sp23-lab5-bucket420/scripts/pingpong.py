import os

sizes = [1, 8, 64, 512, 4096, 32768]
for size in sizes:
    os.system("srun -n 2 -N 1 ./pingpong %d %s" % (size, "pingpong_intra"))
    os.system("srun -n 2 -N 2 ./pingpong %d %s" % (size, "pingpong_inter"))