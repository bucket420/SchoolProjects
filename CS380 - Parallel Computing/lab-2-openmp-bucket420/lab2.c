#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "omp.h"
#include "wctimer.h"

// number of threads
int n_threads = 0;

// initialize matrix
void init2d(double **array2d, int size, double base) {
    *array2d = malloc(size * size * sizeof(double));
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            (*array2d)[i * size + j] = base * (i + 0.1 * j);
        }
    }
}

// zero all elements in matrix
void zero(double *matrix, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i * size + j] = 0.0;
        }
    }
}

// matrix multiplication (ijk)
void mxm_ijk(double *matrix1, double *matrix2, double *result, int size) {
    #pragma omp parallel for num_threads(n_threads)
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < size; k++) {
                result[i * size + j] += matrix1[i * size + k] * matrix2[k * size + j];
            }
        }
    }
}

// matrix multiplication (ikj)
void mxm_ikj(double *matrix1, double *matrix2, double *result, int size) {
    #pragma omp parallel for num_threads(n_threads)
    for (int i = 0; i < size; i++) {
        for (int k = 0; k < size; k++) {
            for (int j = 0; j < size; j++) {
                result[i * size + j] += matrix1[i * size + k] * matrix2[k * size + j];
            }
        }
    }
}

// matrix multiplication (jki)
void mxm_jki(double *matrix1, double *matrix2, double *result, int size) {
    #pragma omp parallel for num_threads(n_threads)
    for (int j = 0; j < size; j++) {
        for (int k = 0; k < size; k++) {
            for (int i = 0; i < size; i++) {
                result[i * size + j] += matrix1[i * size + k] * matrix2[k * size + j];
            }
        }
    }
}

// matrix multiplication with optimization
void mxm2(double *matrix1, double *matrix2, double *result, int size) {
    #pragma omp parallel for num_threads(n_threads)
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < size; k++) {
                result[i * size + j] += matrix1[i * size + k] * matrix2[j * size + k];
            }
        }
    }
}

// transpose matrix
void mmT(double *matrix, int size) {
    double tmp;
    #pragma omp parallel for private(tmp) num_threads(n_threads)
    for (int i = 0; i < size; i++) {
        for (int j = i + 1; j < size; j++) {
            tmp = matrix[i * size + j];
            matrix[i * size + j] = matrix[j * size + i];
            matrix[j * size + i] = tmp;
        }
    }
}

// calculate norm
double normf(double *matrix, int size) {
    double sum = 0.0;
    #pragma omp parallel for reduction(+:sum) num_threads(n_threads)
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            sum += matrix[i * size + j] * matrix[i * size + j];
        }
    }
    return sqrt(sum);
}

int main(int argc, char **argv) {
    // not enough arguments
    if (argc < 3) {
        printf("usage: lab1 <size>\n\t<size>\t size of matrices and vectors\n");
        printf("\t<nthreads>\t number of OpenMP threads to use\n");
        exit(-1);
    }

    // declare variables
    double *A, *B, *C;
    double mxm_ijk_time = 0.0, mxm_ikj_time = 0.0, mxm_jki_time = 0.0, transpose_time = 0.0, mxm2_time = 0.0; 
    int s = atoi(argv[1]);

    // set number of threads
    n_threads = atoi(argv[2]);

    // create and calibrate timer
    wc_timer_t t;
    wc_tsc_calibrate();

    printf("matrix size: %d, number of threads: %d\n", s, n_threads);
    
    // initialize matrices
    init2d(&A, s, 1.0);
    init2d(&B, s, 2.0);
    init2d(&C, s, 0.0);

    // run 10 times
    for (int i = 0; i < 10; i++) {
        // matrix multiplication with mxm_ijk
        zero(C, s);
        WC_INIT_TIMER(t);
        WC_START_TIMER(t);
        mxm_ijk(A, B, C, s);
        WC_STOP_TIMER(t);
        mxm_ijk_time += WC_READ_TIMER_MSEC(t);

        // matrix multiplication with mxm_ikj
        zero(C, s);
        WC_INIT_TIMER(t);
        WC_START_TIMER(t);
        mxm_ikj(A, B, C, s);
        WC_STOP_TIMER(t);
        mxm_ikj_time += WC_READ_TIMER_MSEC(t);

        // matrix multiplication with mxm jki
        zero(C, s);
        WC_INIT_TIMER(t);
        WC_START_TIMER(t);
        mxm_jki(A, B, C, s);
        WC_STOP_TIMER(t);
        mxm_jki_time += WC_READ_TIMER_MSEC(t);

        // transpose B
        WC_INIT_TIMER(t);
        WC_START_TIMER(t);
        mmT(B, s);
        WC_STOP_TIMER(t);
        transpose_time += WC_READ_TIMER_MSEC(t);

        // matrix multiplication with mxm2
        zero(C, s);
        WC_INIT_TIMER(t);
        WC_START_TIMER(t);
        mxm2(A, B, C, s);
        WC_STOP_TIMER(t);
        mxm2_time += WC_READ_TIMER_MSEC(t);
    }

    // compute average time
    mxm2_time = (mxm2_time + transpose_time) / 10.0;
    mxm_ijk_time /= 10.0;
    mxm_ikj_time /= 10.0;
    mxm_jki_time /= 10.0;
    transpose_time /= 10.0;

    // print results
    printf("matrix/matrix multiplication (mxm_ijk): %.7f ms\n", mxm_ijk_time);
    printf("matrix/matrix multiplication (mxm_ikj): %.7f ms\n", mxm_ikj_time);
    printf("matrix/matrix multiplication (mxm_jki): %.7f ms\n", mxm_jki_time);
    printf("matrix transpose: %.7f ms\n", transpose_time);
    printf("matrix/matrix multiplication (mxm2): %.7f ms\n", mxm2_time);
   
    // write results to a csv file
    FILE *csv; 
    char *path_to_file = malloc(15 * sizeof(char));
    sprintf(path_to_file, "./data/%d.csv", s);
    csv = fopen(path_to_file, "a+");
    int c = fgetc(csv);
    if (c == EOF) {
        fprintf(csv, "P, ijk, ikj, jki, mxm2,\n");
    }
    fprintf(csv, "%d, %.3f, %.3f, %.3f, %.3f,\n", n_threads, mxm_ijk_time, mxm_ikj_time, mxm_jki_time, mxm2_time);
    fclose(csv);

    // free allocated memory
    free(A);
    free(B);
    free(C);
    free(path_to_file);
}







