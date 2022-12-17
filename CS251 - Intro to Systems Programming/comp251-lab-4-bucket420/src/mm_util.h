#ifndef __MM_UTIL_H__
#define __MM_UTIL_H__

#include <stddef.h>

typedef struct mm_region_t mm_region_t;
struct mm_region_t {
  void *start;
  size_t size;
  int fd;
};

void mm_open(const char *fname, size_t size, mm_region_t *region);
void mm_close(mm_region_t *region);

#endif
