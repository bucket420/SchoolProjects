#ifndef __TP_TEST_UTILS_H__
#define __TP_TEST_UTILS_H__

#include <pthread.h>

#include "../thread_pool.h"

// in tests that test for conditions periodically, sleep for 0.5ms between
// testing.
#define SLEEP_NS 500000l

threadpool_t make_with(int n);

// functions that atomically check and reset a counter.
void reset_counter();
void wait_for(long target);
void wait_for_gt(long target);
void *inc(void *);
long get_counter();

// sleep for the fixed ns sleep interval
void xsleep();

typedef struct blocking_work_t {
  pthread_mutex_t *mu;
  int *work_signal; // guarded by mu
  int *cb_signal;   // guarded by mu
  // this is called with work_signal and cb_signal. once is called once, and
  // done is called repeatedly after. a nonzero return from done function
  // signals that the work/callback should end.
  // both are called when mu is held.
  void (*once)(int *signal);
  int (*done)(int *signal);
} blocking_work_t;

// make threadpool work with the given work spec.
threadpool_work_t blocking_work(blocking_work_t *work);
// create threadpool work that blocks until n threads have reached the
// work function or the callback.
threadpool_work_t make_block_at_n(int n);
void destroy_block_at_n(threadpool_work_t work);

#endif
