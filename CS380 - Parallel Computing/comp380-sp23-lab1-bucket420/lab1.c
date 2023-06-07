#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "wctimer.h"

void init2d(double **array2d, int size, double base) {
  *array2d = malloc(size * size * sizeof(double));
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      (*array2d)[i * size + j] = base * (i + 0.1 * j);
    }
  }
}

void init1d(double **array1d, int len, double base) {
  *array1d = malloc(len * sizeof(double));
  for (int i = 0; i < len; i++) {
    (*array1d)[i] = base * (i + 1);
  }
}

void mxm(double *matrix1, double *matrix2, double *result, int size) {
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      result[i * size + j] = 0;
      for (int k = 0; k < size; k++) {
        result[i * size + j] += matrix1[i * size + k] * matrix2[k * size + j];
      }
    }
  }
}

void mxm2(double *matrix1, double *matrix2, double *result, int size) {
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      result[i * size + j] = 0;
      for (int k = 0; k < size; k++) {
        result[i * size + j] += matrix1[i * size + k] * matrix2[j * size + k];
      }
    }
  }
}

void mxv(double *matrix, double *vector, double *result, int size) {
 for (int i = 0; i < size; i++) {
    result[i] = 0;
    for (int j = 0; j < size; j++) {
      result[i] += matrix[i * size + j] * vector[j];
    }
  }
}

void mmT(double *matrix, int size) {
  double tmp;
  for (int i = 0; i < size; i++) {
    for (int j = i + 1; j < size; j++) {
      tmp = matrix[i * size + j];
      matrix[i * size + j] = matrix[j * size + i];
      matrix[j * size + i] = tmp;
    }
  }
}

double normf(double *matrix, int size) {
  double sum;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      sum += matrix[i * size + j] * matrix[i * size + j];
    }
  }
  return sqrt(sum);
}

void print_matrix(double *matrix, int size, char* name) {
  if (size >= 5) return;
  printf("matrix %s:\n", name);
  for (int i = 0; i < size; i++) {
    printf("|");
    for (int j = 0; j < size; j++) {
      printf(" %.2f ", matrix[i *size + j]);
    }
    printf("|\n");
  }
  printf("norm:   %.3f\n\n", normf(matrix, size));
}

void print_vector(double *vector, int len, char* name) {
  if (len >= 5) return;
  printf("vector %s:\n", name);
  printf("|");
  for (int i = 0; i < len; i++) {
    printf(" %.2f ", vector[i]);
  }
  printf("|\n\n");
}

int main(int argc, char **argv) {
  // not enough arguments
  if (argc < 2) {
    printf("usage: lab1 <size>\n\t<size>\t size of matrices and vectors\n");
    exit(-1);
  }

  // declare variables
  double *A, *B, *C, *v1, *v2;
  int s = atoi(argv[1]);
 
  // create and calibrate timer
  wc_timer_t t;
  wc_tsc_calibrate();

  // initialize matrices and vectors
  WC_INIT_TIMER(t);
  WC_START_TIMER(t);
  init2d(&A, s, 1.0);
  init2d(&B, s, 2.0);
  init2d(&C, s, 0.0);
  init1d(&v1, s, 1.0);
  init1d(&v2, s, 0.0);
  WC_STOP_TIMER(t);
  printf("matrix/vector initialization: %.7f ms\n", WC_READ_TIMER_MSEC(t));

  // print out matrices and vectors
  printf("initialized matrices and vectors:\n");
  print_matrix(A, s, "A");
  print_matrix(B, s, "B");
  print_matrix(C, s, "C");
  print_vector(v1, s, "v1");
  print_vector(v2, s, "v2");

  // matrix multiplication with mxm
  printf("Computing C = A * B (mxm)\n");
  WC_INIT_TIMER(t);
  WC_START_TIMER(t);
  mxm(A, B, C, s);
  WC_STOP_TIMER(t);
  printf("matrix/matrix multiplication: %.7f ms\n", WC_READ_TIMER_MSEC(t));
  print_matrix(C, s, "C");

  // matrix/vector multiplication
  printf("Computing v2 = B * v1 (mxm)\n");
  WC_INIT_TIMER(t);
  WC_START_TIMER(t);
  mxv(B, v1, v2, s);
  WC_STOP_TIMER(t);
  printf("matrix/vector multiplication: %.7f ms\n", WC_READ_TIMER_MSEC(t));
  print_vector(v2, s, "v2");

  // transpose B
  printf("Computing B'\n");
  WC_INIT_TIMER(t);
  WC_START_TIMER(t);
  mmT(B, s);
  WC_STOP_TIMER(t);
  printf("matrix transpose: %.7f ms\n", WC_READ_TIMER_MSEC(t));
  print_matrix(B, s, "B");

  // matrix multiplication with mxm2
  printf("Computing C = A * B (mxm2)\n");
  WC_INIT_TIMER(t);
  WC_START_TIMER(t);
  mxm2(A, B, C, s);
  WC_STOP_TIMER(t);
  printf("matrix/matrix multiplication: %.7f ms\n", WC_READ_TIMER_MSEC(t));
  print_matrix(C, s, "C");
  
  // free allocated memory
  free(A);
  free(B);
  free(C);
  free(v1);
  free(v2);
}







