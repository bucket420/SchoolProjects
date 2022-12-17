#ifndef __RAND1_H__
#define __RAND1_H__

#include <stdlib.h>
#include <time.h>

// Returns a random in [0...1).
extern inline double rand1() {
  static int is_init = 1;
  if (is_init) {
    srand(time(NULL));
    is_init = 0;
  }
  return ((double)rand()) / RAND_MAX;
}

#endif
