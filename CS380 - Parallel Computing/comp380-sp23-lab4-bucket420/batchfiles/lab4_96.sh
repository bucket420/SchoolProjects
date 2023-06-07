#!/bin/bash
#SBATCH -n 96
#SBATCH -N 2
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/lab4_96.out.%j ../lab4 wDqVEHozzDT1E