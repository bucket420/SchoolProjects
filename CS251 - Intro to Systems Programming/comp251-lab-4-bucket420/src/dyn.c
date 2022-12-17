#include "dyn.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFF_LEN 80
#define SYMBOL_NAME "load_complete"

void *dlh = NULL;
char buffer[BUFF_LEN];
const char *fmt = "%s/phase%d.so";

void call(int n, const char *path) {
  snprintf(buffer, BUFF_LEN, fmt, path, n);
  void *dlh = dlopen(buffer, RTLD_LAZY);
  if (!dlh) {
    fprintf(stderr, "%s\n", dlerror());
    exit(1);
  }
  void *f = dlsym(dlh, SYMBOL_NAME);
  if (!f) {
    fprintf(stderr, "%s\n", dlerror());
    exit(1);
  }
  ((void (*)(void))f)();
  dlclose(dlh);
}
