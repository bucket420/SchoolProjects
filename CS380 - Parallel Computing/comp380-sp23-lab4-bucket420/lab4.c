#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>
#include <openssl/des.h>

// assume that the password is a "number" of base 95 since there are 95 printable ascii characters
#define BASE 95      

// check if the password has been found every 100000 iterations
#define ITERS_PER_COLLECTIVE  100000 

struct global_s {
  int rank;         // rank of the calling process                
  int nproc;        // number of processes
  int lfound;       // bool indicating if the password has been found locally
  int gfound;       // bool indicating if the password has been found globally
  char pw[6];       // buffer to store the password if found
};
typedef struct global_s global_t;

/*
 *  global data structure
 */
global_t g;

/**
 *  base10to95 - convert a decimal number to a base 95 "number" representing the password
 *    @param guess buffer to store the base 95 number
 *    @param length number of digits
 *    @param base10 decimal number
 */
void base10to95(char* guess, int length, long base10) {
  for (int j = length - 1; j >= 0; j--) {
    int digit = base10 % BASE;          // last digit
    guess[j] = digit + 32;              // add 32 to convert to a printable ascii character
    base10 = (base10 - digit) / BASE;   // remove last digit
  }
}

/**
 *  find_pw - interate through all combinations to search for the password
 *    @param length length of password
 *    @param hashed_pw hashed password string
 *    @param salt salt
 */
void find_pw(int length, const char hashed_pw[14], const char salt[3]) {
  char guess[length + 1];    // buffer to store a guess
  guess[length] = '\0';      // null terminate the guess
  char *hashed_guess;        // pointer to the hashed guess

  long n = pow(BASE, length);                   // total number of combinations
  long n_local = ceil((double) n / g.nproc);    // number of combinations checked by each process, rounded up

  // lower and upper bound
  long lower_bound = g.rank * n_local;
  long upper_bound = (g.rank + 1) * n_local;

  // check all combinations on current process (there will be some extras on the last process but that doesn't matter)
  for (long i = lower_bound; i < upper_bound; i++) {
    // generate a guess corresponding to i
    base10to95(guess, length, i);

    // hash the guess 
    hashed_guess = DES_crypt(guess, salt);

    // if the hashed guess matches the hashed password, store the correct guess in g.pw and return 1
    if (!strncmp(hashed_pw, hashed_guess, 13)) {
      strcpy(g.pw, guess);

      // password has been found
      g.lfound = 1;
      g.gfound = 1;

      // if i is far enough from the end, do an all-reduction on gfound
      if (n_local - (i + 1 - lower_bound) + (i + 1 - lower_bound) % ITERS_PER_COLLECTIVE > ITERS_PER_COLLECTIVE) {
        MPI_Allreduce(&g.lfound, &g.gfound, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
      }
      break;
    }
    // check if the password has been found every 100000 iterations
    if (!((i + 1 - lower_bound) % ITERS_PER_COLLECTIVE)) {
      MPI_Allreduce(&g.lfound, &g.gfound, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
      if (g.gfound) {
        break;
      }
    }
  }
}

/**
 * main
 */
int main(int argc, char **argv) {
  double start, end;              // time 
  char hashed_pw[14];             // buffer to store hashed password
  char salt[3];                   // buffer to store hashed salt

  // check number of arguments
  if (argc != 2) {
    printf("\tusage: lab4 <hashed_password>\n");
    exit(1);
  }

  // check length of hashed password
  if (strnlen(argv[1], 13) != 13) {
    printf("hashed password must have %d characters\n", 13);
    exit(-1);
  }

  // store hashed password and salt
  strncpy(hashed_pw, argv[1], 13);
  hashed_pw[13] = '\0';
  strncpy(salt, argv[1], 2);
  salt[2] = '\0';

  // initialize
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &g.rank);
  MPI_Comm_size(MPI_COMM_WORLD, &g.nproc);
  g.lfound = 0;
  g.gfound = 0;

  // print info
  if (g.rank==0) {
    printf("starting run with %d processes\n", g.nproc);
    printf("hashed password: %s, salt: %s\n", hashed_pw, salt);
  }

  // find the password and measure the time
  start = MPI_Wtime();
  find_pw(5, hashed_pw, salt);
  MPI_Barrier(MPI_COMM_WORLD);
  end = MPI_Wtime();

  // print out the time
  if (g.rank == 0) {
    printf("elapsed time: %7.4fs\n", end - start);  
  }

  // print out the password
  if (g.lfound) {
    printf("the password is: %s\n", g.pw);
  }
  
  // finalize
  MPI_Finalize();
}
