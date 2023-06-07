#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <mpi.h>
#include <openssl/des.h>

// tree barrier
void tree_barrier(int rank, int size) {
  int message = rank; 
  MPI_Status status;
  int parent = (rank + 1) / 2 - 1;  // parent 
  int left = 2 * rank + 1;          // left child
  int right = 2 * rank + 2;         // right child

  if (rank && left >= size) { // if this is a leaf node, only send message to parent
    MPI_Send(&message, 1, MPI_INT, parent, 0, MPI_COMM_WORLD);
  } else { // if this is not a leaf node, receive message(s) from child then send message to parent
    if (left < size)
      MPI_Recv(&message, 1, MPI_INT, left, 0, MPI_COMM_WORLD, &status);
    if (right < size)
      MPI_Recv(&message, 1, MPI_INT, right, 0, MPI_COMM_WORLD, &status);
    if (rank) 
      MPI_Send(&message, 1, MPI_INT, parent, 0, MPI_COMM_WORLD);
  }

  if (!rank) { // if this is the root, only send messages to children
    if (left < size)
      MPI_Send(&message, 1, MPI_INT, left, 1, MPI_COMM_WORLD);
    if (right < size)
      MPI_Send(&message, 1, MPI_INT, right, 1, MPI_COMM_WORLD);
  } else { // if this is not the root, receive message from parent then send message(s) to child(ren)
    MPI_Recv(&message, 1, MPI_INT, parent, 1, MPI_COMM_WORLD, &status);
    if (left < size)
      MPI_Send(&message, 1, MPI_INT, left, 1, MPI_COMM_WORLD);
    if (right < size)
      MPI_Send(&message, 1, MPI_INT, right, 1, MPI_COMM_WORLD);
  }
}

// main
int main(int argc, char **argv) {
  double start, end;
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (rank == 0)
    printf("starting run with %d processes\n", size);

  // time measurement
  double time = 0.0;
  for (int i = 0; i < 1000; i++) {
    start = MPI_Wtime();
    tree_barrier(rank, size);
    end = MPI_Wtime();
    time += end - start;
  }
 
  // print out the results and write them to a csv file
  if (rank == 0) {
    printf("elapsed time each barrier: %7.4f microseconds\n", time * 1000.0);

    FILE *csv; 
    char *path_to_file = malloc(50 * sizeof(char));
    sprintf(path_to_file, "./data/barrier-tree.csv");
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
