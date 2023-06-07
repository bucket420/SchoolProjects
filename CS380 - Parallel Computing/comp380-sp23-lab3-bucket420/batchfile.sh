#!/bin/bash
#SBATCH -n 2112
#SBATCH -N 44
#SBATCH --ntasks-per-node=48

srun --mpi=pmi2 -o ./output/with_scalar_tuning/2112.out.%j ./lab3 -n 300000 -t 5
