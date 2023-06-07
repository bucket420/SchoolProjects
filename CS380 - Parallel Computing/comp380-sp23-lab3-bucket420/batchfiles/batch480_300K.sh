#!/bin/bash
#SBATCH -n 480
#SBATCH -N 10
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/matrix/480_300K.out.%j ../lab3_matrix -n 300000 -t 5
