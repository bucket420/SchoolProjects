#ifndef __UTIL_H__
#define __UTIL_H__

#include <pthread.h>
#include <stdio.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define assertz(expr) assert((expr) == 0)

#ifdef DEBUG

#include <assert.h>

#define DEBUG_PRINT(format, args...)                                           \
  fprintf(stderr, "[D (%lx) %s:%d:%s()] " format, pthread_self(), __FILE__,    \
          __LINE__, __func__, ##args)

#else

#define DEBUG_PRINT(fmt, args...) // no-op

#define assert(expr)                                                           \
  if (!(expr)) {                                                               \
    fprintf(stderr, "Error %s:%d:%s: %s\n", __FILE__, __LINE__, __func__,      \
            "" #expr "");                                                      \
    __asm__ __volatile__("int $3");                                            \
  }

#endif

#endif
