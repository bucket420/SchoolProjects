#include "disk_array.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_LEN 80

void usage() {
  printf("m name size    make new array\n"
         "s idx element  set element\n"
         "g idx          get element\n"
         "c              close table\n"
         "p              print elements\n"
         "q              quit\n");
}

uint64_t parse(const char *str) { return strtoull(str, NULL, 10); }

int main(int argc, char **argv) {

  char input[BUFF_LEN];
  int tmp_int;
  char *tmp_str;
  disk_array_t arr;
  uint64_t *data = NULL;

  usage();
  printf("> ");
  while (fgets(input, BUFF_LEN, stdin)) {
    int len = strlen(input);
    input[--len] = '\0';
    char *op = strtok(input, " ");
    char *str = strtok(NULL, " ");
    switch (*op) {
    case 'm':
      tmp_str = strtok(NULL, " ");
      tmp_int = 0;
      if (tmp_str) {
        tmp_int = atoi(tmp_str);
      }
      printf("Opening file %s (size %d)\n", str, tmp_int);
      array_open(str, tmp_int, (uint64_t)sizeof(uint64_t), &arr);
      data = arr.array;
      printf("array start: %p\n", data);
      printf("array size: %lu\n", *arr.n);
      break;
    case 's':
      tmp_int = atoi(str);
      data[tmp_int] = parse(strtok(NULL, " "));
      printf("added element %s; %s (%p)\n", str, tmp_str ? "OK" : "FAILED",
             tmp_str);
      break;
    case 'g':
      tmp_int = atoi(str);
      printf("get element %d: %lu\n", tmp_int, data[tmp_int]);
      break;
    case 'c':
      printf("closing...\n");
      array_close(&arr);
      break;
    case 'p':
      for (int i = 0; i < *arr.n; i++) {
        printf("%s%lu%s", i == 0 ? "[" : "", data[i],
               i < *arr.n - 1 ? ", " : "]\n");
      }
      break;
    case 'q':
    case 'e':
      return 0;
    case '?':
    default:
      usage();
      break;
    }
    printf("> ");
  }
}
