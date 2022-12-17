#include "strtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_LEN 80

void usage() {
  printf("n name size    create new table\n"
         "a element      append element\n"
         "g index        get element\n"
         "c              close table\n"
         "l index        get element length\n"
         "s              get table length\n"
         "q              quit\n");
}

int main(int argc, char **argv) {

  char input[BUFF_LEN];
  int tmp_int;
  char *tmp_str;
  strtable_t tbl;

  usage();
  printf("> ");
  while (fgets(input, BUFF_LEN, stdin)) {
    int len = strlen(input);
    input[--len] = '\0';
    char *op = strtok(input, " ");
    char *str = strtok(NULL, " ");
    switch (*op) {
    case 'n':
      tmp_str = strtok(NULL, " ");
      tmp_int = 0;
      if (tmp_str) {
        tmp_int = atoi(tmp_str);
      }
      printf("Opening file %s (size %d)\n", str, tmp_int);
      strtable_open(str, tmp_int, &tbl);
      printf("table offset: %p\n", tbl.metadata);
      break;
    case 'a':
      tmp_str = add_element(&tbl, str);
      printf("added element %s; %s (%p)\n", str, tmp_str ? "OK" : "FAILED",
             tmp_str);
      break;
    case 'g':
      tmp_int = atoi(str);
      tmp_str = get_element(&tbl, tmp_int);
      printf("get element %d: %s (%p)\n", tmp_int, tmp_str ? tmp_str : "-",
             tmp_str);
      break;
    case 'c':
      printf("closing table...\n");
      strtable_close(&tbl);
      break;
    case 'l':
      tmp_int = atoi(str);
      printf("element %d length: %d\n", tmp_int,
             get_element_len(&tbl, tmp_int));
      break;
    case 's':
      printf("table len: %d\n", strtable_len(&tbl));
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
