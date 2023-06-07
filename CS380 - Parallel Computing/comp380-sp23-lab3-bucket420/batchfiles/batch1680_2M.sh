#!/bin/bash
#SBATCH -n 1680
#SBATCH -N 35
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/matrix/1680_2M.out.%j ../lab3_matrix -n 2000000 -t 5
