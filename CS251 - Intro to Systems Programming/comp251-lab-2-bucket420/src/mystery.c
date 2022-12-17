#include <stdbool.h>
#include <string.h>

#include "phases.h"

int main(int argc, char **argv) {
  int quiet = 0;
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "-quiet", 6) == 0) {
      quiet |= PHASE_SKIP;
    } else if (strncmp(argv[i], "-skip", 5) == 0) {
      quiet |= INTRO_SKIP;
    }
  }
  run_phases(quiet);
  return 0;
}
