#!/bin/bash
#SBATCH -n 48
#SBATCH -N 1
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/lab4_48.out.%j ../lab4 wDqVEHozzDT1E