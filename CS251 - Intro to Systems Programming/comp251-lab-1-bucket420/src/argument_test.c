#include <stdio.h>

#define MAX_FLAGS 512

int main(int argc, char **argv) {
  printf("Got %d arguments:\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("\t%s\n", argv[i]);
  }
  return 0;
}
