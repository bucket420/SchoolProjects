#!/bin/bash
#SBATCH -n 192
#SBATCH -N 4
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/lab4_192.out.%j ../lab4 wDqVEHozzDT1E