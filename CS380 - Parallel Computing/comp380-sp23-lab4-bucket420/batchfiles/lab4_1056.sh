#!/bin/bash
#SBATCH -n 1056
#SBATCH -N 22
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/lab4_1056.out.%j ../lab4 wDqVEHozzDT1E