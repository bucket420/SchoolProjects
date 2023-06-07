#!/bin/bash
#SBATCH -n 960
#SBATCH -N 20
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/matrix/960_300K.out.%j ../lab3_matrix -n 300000 -t 5
