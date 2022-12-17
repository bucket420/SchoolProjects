#include "tp_test_utils.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "../thread_pool.h"
#include "../util.h"

threadpool_t make_with(int n) {
  threadpool_config_t cfg;
  cfg.nthreads = n;
  return threadpool_create(cfg);
}

void xsleep() {
  struct timespec r = {0, SLEEP_NS};
  nanosleep(&r, NULL);
}

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
long counter;

void reset_counter() {
  pthread_mutex_lock(&mu);
  counter = 0;
  pthread_mutex_unlock(&mu);
}

void *inc(void *unused) {
  pthread_mutex_lock(&mu);
  counter += 1;
  long ret = counter;
  DEBUG_PRINT("incremented counter %ld\n", counter);
  pthread_mutex_unlock(&mu);
  return (void *)ret;
}

void wait_for(long target) {
  int stop = 0;
  while (!stop) {
    pthread_mutex_lock(&mu);
    stop = counter == target;
    pthread_mutex_unlock(&mu);
    xsleep();
  }
}

void wait_for_gt(long target) {
  int stop = 0;
  while (!stop) {
    pthread_mutex_lock(&mu);
    stop = counter >= target;
    pthread_mutex_unlock(&mu);
    xsleep();
  }
}

long get_counter() {
  long cval;
  pthread_mutex_lock(&mu);
  cval = counter;
  pthread_mutex_unlock(&mu);
  return cval;
}

// TODO: refactor these
void blocking_callback(void *arg) {
  blocking_work_t *w = arg;
  int stop = 0;
  pthread_mutex_lock(w->mu);
  w->once(w->cb_signal);
  pthread_mutex_unlock(w->mu);
  while (!stop) {
    pthread_mutex_lock(w->mu);
    stop = w->done(w->cb_signal);
    pthread_mutex_unlock(w->mu);
    xsleep();
  }
}

void *blocking_work_fn(void *arg) {
  blocking_work_t *w = arg;
  int stop = 0;
  pthread_mutex_lock(w->mu);
  w->once(w->work_signal);
  pthread_mutex_unlock(w->mu);
  while (!stop) {
    pthread_mutex_lock(w->mu);
    stop = w->done(w->work_signal);
    pthread_mutex_unlock(w->mu);
    xsleep();
  }
  return w;
}

threadpool_work_t blocking_work(blocking_work_t *work) {
  threadpool_work_t w = {
      blocking_work_fn,
      work,
      blocking_callback,
  };
  return w;
}

void inc_once(int *arr) {
  arr[0]++;
  inc(NULL);
}

int wait_for_n(int *arr) {
  int cur = arr[0];
  int target = arr[1];
  return cur == target;
}

threadpool_work_t make_block_at_n(int n) {
  blocking_work_t *work = malloc(sizeof(blocking_work_t));
  work->mu = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(work->mu, NULL);
  work->cb_signal = calloc(2, sizeof(int));
  work->work_signal = calloc(2, sizeof(int));
  work->cb_signal[1] = n;
  work->work_signal[1] = n;
  work->done = wait_for_n;
  work->once = inc_once;
  threadpool_work_t w = blocking_work(work);
  return w;
}

void destroy_block_at_n(threadpool_work_t work) {
  blocking_work_t *w = work.work;
  pthread_mutex_destroy(w->mu);
  free(w->mu);
  free(w->cb_signal);
  free(w->work_signal);
  free(w);
}
