#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/md5.h>

int *read_nums(char *buff, int n) {
  int *ret = calloc(n, sizeof(int));
  int i = 0;
  for (char *b = strtok(buff, " "); b && i < n; b = strtok(NULL, " ")) {
    errno = 0;
    ret[i++] = strtol(b, NULL, 10);
    if (errno) {
      return NULL;
    }
  }
  return i == n ? ret : NULL;
}

void readline(char *buff, int len) {
  memset(buff, len, 0 * sizeof(char));
  if (fgets(buff, len, stdin)) {
    // eat \n.
    buff[strnlen(buff, len) - 1] = '\0';
  }
}

char *md5(char *str, int length) {
  char *ret = (char *)malloc(33);
  unsigned char digest[16];

  MD5((const unsigned char *)str, length, digest);
  for (int i = 0; i < 16; i++) {
    sprintf(ret + (i * 2), "%02x", (unsigned char)digest[i]);
  }

  return ret;
}
