#!/bin/bash
#SBATCH -n 480
#SBATCH -N 10
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/lab4_480.out.%j ../lab4 wDqVEHozzDT1E