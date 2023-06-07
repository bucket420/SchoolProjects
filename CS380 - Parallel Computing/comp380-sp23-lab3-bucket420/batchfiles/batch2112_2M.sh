#!/bin/bash
#SBATCH -n 2112
#SBATCH -N 44
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/matrix/2112_2M.out.%j ../lab3_matrix -n 2000000 -t 5
