#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <mpi.h>
#include <openssl/des.h>

// ring barrier
void ring_barrier(int rank, int size) {
  int message = rank; 
  MPI_Status status;
  MPI_Request send_req;
  MPI_Request recv_req;
  int next = (rank + 1) % size;         // next processor in the ring
  int prev = (rank + size - 1) % size;  // previous processor in the ring
  for (int p = 0; p < size; p++) {
    MPI_Isend(&message, 1, MPI_INT, next, p, MPI_COMM_WORLD, &send_req);
    MPI_Irecv(&message, 1, MPI_INT, prev, p, MPI_COMM_WORLD, &recv_req);
    MPI_Wait(&send_req, &status);
    MPI_Wait(&recv_req, &status);
  }
}

// main
int main(int argc, char **argv) {
  double start, end;
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank==0)
    printf("starting run with %d processes\n", size);

  // time measurement
  double time = 0.0;
  for (int i = 0; i < 1000; i++) {
    start = MPI_Wtime();
    ring_barrier(rank, size);
    end = MPI_Wtime();
    time += end - start;
  }
 
  // print out the results and write them to a csv file
  if (rank==0) {
    printf("elapsed time each barrier: %7.4f microseconds\n", time * 1000.0);

    FILE *csv; 
    char *path_to_file = malloc(50 * sizeof(char));
    sprintf(path_to_file, "./data/barrier-ring.csv");
    csv = fopen(path_to_file, "a+");
    int c = fgetc(csv);
    if (c == EOF) {
        fprintf(csv, "p, time\n");
    }
    fprintf(csv, "%d, %.4f\n", size, time * 1000);
    fclose(csv);

    free(path_to_file);
  }

  // close
  MPI_Finalize();
}
