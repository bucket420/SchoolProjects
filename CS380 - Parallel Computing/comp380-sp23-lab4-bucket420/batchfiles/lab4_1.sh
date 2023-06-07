#!/bin/bash
#SBATCH -n 1
#SBATCH -t 10:00:00

srun --mpi=pmi2 -o ../output/lab4_1.out.%j ../lab4 wDqVEHozzDT1E