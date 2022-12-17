#ifndef __FNV64_H__
#define __FNV64_H__

#include <stdint.h>

extern inline int64_t fnv64(const char *str) {
  int64_t h = 0xcbf29ce484222325;
  for (const char *s = str; *s; s++) {
    h = h + (h << 1) + (h << 4) + (h << 5) + (h << 7) + (h << 8) + (h << 40);
    h = h ^ *s;
  }
  return h;
}

#endif
