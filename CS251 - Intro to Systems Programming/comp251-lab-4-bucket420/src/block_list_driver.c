#include "block_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_LEN 80

void usage() {
  printf("m name size      make new list\n"
         "a size c         append element w/ given sizee\n"
         "n                next element\n"
         "p                prev element\n"
         "r                reset iterator\n"
         "c                close list\n"
         "q                quit\n");
}

uint64_t parse(const char *str) { return strtoull(str, NULL, 10); }

int main(int argc, char **argv) {

  char input[BUFF_LEN];
  uint32_t tmp_int;
  char *tmp_str;
  char *last = NULL;
  block_list_t lst;
  char tmp_char;

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
      bl_open(str, tmp_int, &lst);
      break;
    case 'a':
      tmp_int = atoi(str);
      tmp_str = strtok(NULL, " ");
      tmp_char = *tmp_str;
      tmp_str = malloc(tmp_int);
      memset(tmp_str, tmp_char, tmp_int);
      tmp_str = bl_append(tmp_str, tmp_int, &lst);
      if (tmp_str) {
        printf("appended element (%d * %c)\n", tmp_int, tmp_char);
      } else {
        printf("could not append %d of %c!\n", tmp_int, tmp_char);
      }
      break;
    case 'r':
      last = NULL;
      printf("reset iterator\n");
      break;
    case 'p':
      tmp_str = bl_prev(last, &tmp_int, &lst);
      printf("next: %d of %c (end = %c)\n", tmp_int,
             tmp_str != NULL ? *tmp_str : 'X', tmp_str != NULL ? 'N' : 'Y');
      last = tmp_str;
      break;
    case 'n':
      tmp_str = bl_next(last, &tmp_int, &lst);
      printf("next: %d of %c (end = %c)\n", tmp_int,
             tmp_str != NULL ? *tmp_str : 'X', tmp_str != NULL ? 'N' : 'Y');
      last = tmp_str;
      break;
    case 'c':
      printf("closing...\n");
      bl_close(&lst);
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
