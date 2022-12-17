#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "map.h"
#include "rand1.h"

#define DEFAULT_SIZE 256
#define DEFAULT_RESIZE 0.0
#define DEFAULT_KEYS 1024
#define DEFAULT_ITERATIONS 4
#define DEFAULT_OPERATION_MULTIPLIER 4

// 2^32 is 10 digits
#define BUFF_SIZE 11

void usage() {
  printf("Usage: stress_test [-s <size>] [-k <keys>] [-r <prob>] [-i <iters>] "
         "[-m <ops>]\n"
         "Options:\n"
         "\t-r <prob>    probability of resize (0-100)\n"
         "\t-s <size>    size of map\n"
         "\t-i <iters>   iterations\n"
         "\t-m <ops>     operations per key\n"
         "\t-k <keys>    number of keys\n");
}

void print_stats(map_t *map) {
  map_metrics_t *metrics = map_metrics(map);
  printf("num_entries:\t%d\n", metrics->num_entries);
  printf("max_depth:\t%d\n", metrics->max_depth);
  printf("size:\t\t%d\n", metrics->curr_size);
  free(metrics);
}

int main(int argc, char **argv) {
  uint32_t size = DEFAULT_SIZE;
  double resize_probability = DEFAULT_RESIZE;
  uint32_t keys = DEFAULT_KEYS;
  uint32_t iterations = DEFAULT_ITERATIONS;
  uint32_t ops = DEFAULT_OPERATION_MULTIPLIER;
  char c;
  srand(time(NULL));

  char buff[BUFF_SIZE]; // buffer for keys

  map_t *map = NULL;

  while ((c = getopt(argc, argv, "hr:s:k:i:m:")) != EOF) {
    switch (c) {
    case 'r':
      resize_probability = atoi(optarg) / 100.0;
      break;
    case 'm':
      ops = atoi(optarg);
      break;
    case 'k':
      keys = atoi(optarg);
      break;
    case 's':
      size = atoi(optarg);
      break;
    case 'i':
      iterations = atoi(optarg);
      break;
    case 'h': // fallthrough intentional
    default:
      usage();
      exit(1);
    }
  }

  uint32_t test_iters = ops * keys;

  // create values, indexed by numeric key. initial value is string
  // representation of key.
  //
  // we own the values in the map, so we will update these values, simulating a
  // replace.
  char **vals = malloc(sizeof(char *) * keys);
  for (int i = 0; i < keys; i++) {
    vals[i] = malloc(BUFF_SIZE);
    sprintf(vals[i], "%d", i);
  }

  // run n iterations of the stress test
  for (int i = 0; i < iterations; i++) {
    printf("-------------------------------------\n"
           "    Iteration %d / %d (%d iters)\n"
           "-------------------------------------\n",
           i, iterations, test_iters);
    // first, allocate the map.
    map = map_create(size);

    for (int ii = 0; ii < test_iters; ii++) {
      // pick a key.
      int key = rand1() * keys;
      char *val = vals[key];
      sprintf(buff, "%d", key);

      // put it or remove it.
      if (rand1() < 0.2) {
        printf("r");
        map_remove(map, buff);
      } else {
        printf("p");
        map_put(map, buff, val);
      }

      // try a resize.
      if (rand1() < resize_probability) {
        printf("n");
        map_resize(map, rand1() * (size * 2) + 1);
      }
    }

    printf("...done.\n");

    print_stats(map);

    // finally, free it.
    map_free(&map);
    assert(!map); // map should be null
  }

  // clean up after ourselves
  for (int i = 0; i < keys; i++) {
    free(vals[i]);
  }
  free(vals);
}
