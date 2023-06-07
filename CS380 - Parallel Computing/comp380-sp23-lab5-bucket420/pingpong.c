#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <mpi.h>
#include <openssl/des.h>

// initialize message buffer
void init(double *message, int s) {
  for (int i = 0; i < s; i++) {
    message[i] = i + 0.1;
  }
}

// main
int main(int argc, char **argv) {
  double start, end;
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc < 3) {
    if (rank == 0) {
      printf("usage: pingpong <size> <output name>\n");
    }
  }

  if (rank == 0)
    printf("starting run with %d processes\n", size);

  // initialize variables
  int s = atoi(argv[1]); // message size
  double *message = malloc(s * sizeof(double));
  init(message, s);
  double time = 0.0;
  double time_nb = 0.0;
  double time_nb_send = 0.0;
  double time_nb_recv = 0.0;
  MPI_Status status;
  MPI_Request send_req;
  MPI_Request recv_req;

  // blocking
  for (int i = 0; i < 1000; i++) {
    start = MPI_Wtime();
    if (rank == 0) {
      MPI_Send(message, s, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
      MPI_Recv(message, s, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
    } else {
      MPI_Recv(message, s, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
      MPI_Send(message, s, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }
    end = MPI_Wtime();
    time += end - start;
  }

  // non-blocking
  for (int i = 0; i < 1000; i++) {
    start = MPI_Wtime();
    if (rank == 0) {
      MPI_Isend(message, s, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &send_req);
      MPI_Irecv(message, s, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &recv_req);
      MPI_Wait(&send_req, &status);
      MPI_Wait(&recv_req, &status);
    } else {
      MPI_Isend(message, s, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &send_req);
      MPI_Irecv(message, s, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &recv_req);
      MPI_Wait(&send_req, &status);
      MPI_Wait(&recv_req, &status);
    }
    end = MPI_Wtime();
    time_nb += end - start;
  }

  // non-blocking send, blocking receive
  for (int i = 0; i < 1000; i++) {
    start = MPI_Wtime();
    if (rank == 0) {
      MPI_Isend(message, s, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &send_req);
      MPI_Recv(message, s, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
      MPI_Wait(&send_req, &status);
    } else {
      MPI_Isend(message, s, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &send_req);
      MPI_Recv(message, s, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
      MPI_Wait(&send_req, &status);
    }
    end = MPI_Wtime();
    time_nb_send += end - start;
  }

  // blocking send, non-blocking receive
  for (int i = 0; i < 1000; i++) {
    start = MPI_Wtime();
    if (rank == 0) {
      MPI_Irecv(message, s, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &recv_req);
      MPI_Send(message, s, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
      MPI_Wait(&recv_req, &status);
    } else {
      MPI_Irecv(message, s, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &recv_req);
      MPI_Send(message, s, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
      MPI_Wait(&recv_req, &status);
    }
    end = MPI_Wtime();
    time_nb_recv += end - start;
  }
 
  // print out the results and write them to a csv file
  if (rank==0) {
    printf("elapsed time each message (blocking): %7.4f microseconds\n", time * 1000.0);
    printf("elapsed time each message (non-blocking): %7.4f microseconds\n", time_nb * 1000.0);
    printf("elapsed time each message (non-blocking send): %7.4f microseconds\n", time_nb_send * 1000.0);
    printf("elapsed time each message (non-blocking receive): %7.4f microseconds\n", time_nb_recv * 1000.0);

    FILE *csv; 
    char *path_to_file = malloc(50 * sizeof(char));
    sprintf(path_to_file, "./data/%s.csv", argv[2]);
    csv = fopen(path_to_file, "a+");
    int c = fgetc(csv);
    if (c == EOF) {
        fprintf(csv, "size, blocking, non-blocking, non-blocking send, non-blocking receive\n");
    }
    fprintf(csv, "%d, %.4f, %.4f, %.4f, %.4f\n", s, time * 1000, time_nb * 1000, time_nb_send * 1000, time_nb_recv * 1000);
    fclose(csv);

    free(path_to_file);
  }

  // free memory and close
  free(message);
  MPI_Finalize();
}
