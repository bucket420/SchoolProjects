#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boot.h"

#define DYNLIB_PATH "/opt/251/lab4-dyn"
#define PARAMS_PATH "db/params"
#define DB_PATH "db/nav"
#define LOG_PATH "db/log"

int main(int argc, char **argv) {
  int quiet = 0;
  struct boot_params params = {DYNLIB_PATH, PARAMS_PATH, DB_PATH, LOG_PATH};
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-quiet")) {
      quiet |= QUIET_SKIP_IO;
    } else if (!strcmp(argv[i], "-skip")) {
      quiet |= QUIET_SKIP_INTRO;
    } else if (!strncmp(argv[i], "-dynpath=", 7) ||
               !strncmp(argv[i], "-f=", 3)) {
      params.dynlib_path = strstr(argv[i], "=") + 1;
    } else if (!strncmp(argv[i], "-ppath=", 7)) {
      params.params_path = strstr(argv[i], "=") + 1;
    } else if (!strncmp(argv[i], "-dpath=", 7)) {
      params.db_path = strstr(argv[i], "=") + 1;
    } else if (!strncmp(argv[i], "-lpath=", 7)) {
      params.log_path = strstr(argv[i], "=") + 1;
    } else {
      printf("Unknown argument %s\n", argv[i]);
      exit(1);
    }
  }
  boot(params, quiet);
  return 0;
}
