#!/bin/bash
#SBATCH -n 2112
#SBATCH -N 44
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/lab4_2112.out.%j ../lab4 wDqVEHozzDT1E