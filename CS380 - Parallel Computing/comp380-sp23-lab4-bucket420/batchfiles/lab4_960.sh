#!/bin/bash
#SBATCH -n 960
#SBATCH -N 20
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ../output/lab4_960.out.%j ../lab4 wDqVEHozzDT1E