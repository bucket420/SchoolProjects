#ifndef __BOOT_H__
#define __BOOT_H__

#define QUIET_SKIP_IO 1
#define QUIET_SKIP_INTRO 2

struct boot_params {
  char *dynlib_path;
  char *params_path;
  char *db_path;
  char *log_path;
};

void boot(struct boot_params params, int quiet);

#endif
