#!/bin/bash
#SBATCH -n 8

srun --mpi=pmi2 -o ../output/lab4_8.out.%j ../lab4 wDqVEHozzDT1E