#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  char *path;
  path = getenv("PATH");
  if (path == NULL) {
    printf("No inherited environment!\n");
    exit(1);
  }

  char *tmp = path;
  int npath = 0;
  tmp = strchr(tmp, ':');
  while (tmp != NULL) {
    npath++;
    tmp = strchr(tmp + 1, ':');
  }
  if (npath < 1) {
    printf("path separators not found...\n");
    exit(1);
  }
  printf("OK %d %s\n", npath, path);
  return 0;
}
