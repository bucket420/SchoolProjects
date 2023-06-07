#!/bin/bash
#SBATCH -n 1680
#SBATCH -N 35
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/lab4_1680.out.%j ../lab4 wDqVEHozzDT1E