#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <mpi.h>
#include <openssl/des.h>

void init(double *message, int rank, int s) {
  for (int i = 0; i < s; i++) {
    message[i] = i * (rank + 1) + 0.1;
  }
}

// main
int main(int argc, char **argv) {
  double start, end;
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc < 2) {
    if (rank == 0) {
      printf("usage: ring <size>\n");
    }
  }

  if (rank==0)
    printf("starting run with %d processes\n", size);

  // initialize variables
  int s = atoi(argv[1]); // message size
  double *message = malloc(s * sizeof(double)); 
  init(message, rank, s);
  double time = 0.0;
  MPI_Status status;
  MPI_Request send_req;
  MPI_Request recv_req;

  // time measurement
  for (int i = 0; i < 1000; i++) {
    start = MPI_Wtime();
    int next = (rank + 1) % size;         // next processor in the ring
    int prev = (rank + size - 1) % size;  // previous processor in the ring
    for (int p = 0; p < size; p++) {
      MPI_Isend(message, s, MPI_DOUBLE, next, p, MPI_COMM_WORLD, &send_req);
      MPI_Irecv(message, s, MPI_DOUBLE, prev, p, MPI_COMM_WORLD, &recv_req);
      MPI_Wait(&send_req, &status);
      MPI_Wait(&recv_req, &status);
    }
    end = MPI_Wtime();
    time += end - start;
  }
 
  if (rank==0) {
    printf("elapsed time each round: %7.4f microseconds\n", time * 1000.0);

    FILE *csv; 
    char *path_to_file = malloc(50 * sizeof(char));
    sprintf(path_to_file, "./data/ring.csv");
    csv = fopen(path_to_file, "a+");
    int c = fgetc(csv);
    if (c == EOF) {
        fprintf(csv, "size, p, time\n");
    }
    fprintf(csv, "%d, %d, %.4f\n", s, size, time * 1000);
    fclose(csv);

    free(path_to_file);
  }

  free(message);

  // close
  MPI_Finalize();
}
