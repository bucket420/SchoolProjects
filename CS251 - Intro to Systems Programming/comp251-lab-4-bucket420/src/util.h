#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>

#ifdef DEBUG

#include <assert.h>

#define DEBUG_PRINT(format, args...)                                           \
  fprintf(stderr, "D %s:%d:%s(): " format, __FILE__, __LINE__, __func__, ##args)

#else

#define DEBUG_PRINT(fmt, args...) // no-op

#define assert(expr)                                                           \
  if (!(expr))                                                                 \
    fprintf(stderr, "Error: " #expr "\n");

#endif

#endif
