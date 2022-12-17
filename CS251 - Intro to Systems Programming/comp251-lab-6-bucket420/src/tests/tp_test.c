#include <pthread.h>
#include <stdlib.h>

#include "../thread_pool.h"
#include "test_utils.h"
#include "tp_test_utils.h"

int test_create() {
  threadpool_t tp = make_with(5);
  EXPECT_NOTNULL(tp);
  threadpool_destroy(tp);
  return 0;
}

int test_run_one() {
  threadpool_t tp = make_with(5);
  reset_counter();
  threadpool_work_t w = {inc, NULL, NULL};
  threadpool_start(tp);
  threadpool_add(tp, w);
  wait_for(1);
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_destroy(tp);
  return 0;
}

int test_double_start() {
  threadpool_t tp = make_with(5);
  reset_counter();
  threadpool_work_t w = {inc, NULL, NULL};
  int state = threadpool_start(tp);
  EXPECT_INT_EQ(THREADPOOL_STATE_RUNNING, state);
  state = threadpool_start(tp);
  EXPECT_INT_EQ(THREADPOOL_STATE_RUNNING, state);
  threadpool_add(tp, w);
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_destroy(tp);
  return 0;
}

int test_double_stop() {
  threadpool_t tp = make_with(5);
  reset_counter();
  threadpool_work_t w = {inc, NULL, NULL};
  int state = threadpool_start(tp);
  EXPECT_INT_EQ(THREADPOOL_STATE_RUNNING, state);
  threadpool_add(tp, w);
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_destroy(tp);
  return 0;
}

int test_run_many() {
  threadpool_t tp = make_with(5);
  reset_counter();
  threadpool_work_t w = {inc, NULL, NULL};
  threadpool_start(tp);
  int iters = 1000;
  for (int i = 0; i < iters; i++) {
    threadpool_add(tp, w);
  }
  wait_for(iters);
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_destroy(tp);
  return 0;
}

int test_counters() {
  threadpool_t tp = make_with(5);
  reset_counter();
  threadpool_work_t w = {inc, NULL, NULL};
  int iters = 1000;
  for (int i = 0; i < iters; i++) {
    threadpool_add(tp, w);
  }
  threadpool_counters_t counters = threadpool_counters(tp);
  EXPECT_INT_EQ(0, counters.completed_work);
  EXPECT_INT_EQ(iters, counters.queued_work);
  threadpool_start(tp);
  wait_for(iters);
  counters = threadpool_counters(tp);
  EXPECT_INT_EQ(iters, counters.completed_work);
  EXPECT_INT_EQ(0, counters.queued_work);
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  counters = threadpool_counters(tp);
  EXPECT_INT_EQ(iters, counters.completed_work);
  EXPECT_INT_EQ(0, counters.queued_work);
  threadpool_destroy(tp);
  return 0;
}

int test_queue_work() {
  threadpool_t tp = make_with(5);
  reset_counter();
  threadpool_work_t w = {inc, NULL, NULL};
  int iters = 100;
  for (int i = 0; i < iters; i++) {
    threadpool_add(tp, w);
  }
  // start after queueing a bunch of work.
  threadpool_start(tp);
  wait_for(iters);
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_destroy(tp);
  return 0;
}

int test_many_threads() {
  int nthreads = 1000;
  threadpool_t tp = make_with(nthreads);
  reset_counter();
  threadpool_work_t w = {inc, NULL, NULL};
  int iters = 100 * nthreads;
  for (int i = 0; i < iters; i++) {
    threadpool_add(tp, w);
  }
  // start after queueing a bunch of work.
  threadpool_start(tp);
  wait_for(iters);
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_destroy(tp);
  return 0;
}

int test_drain() {
  int nthreads = 2;
  // this test is time-dependent, since we try to stop the pool immediately
  // after starting. ideally we would inject a way to delay the pool from
  // starting work.
  int work = nthreads * 10000;
  threadpool_t tp = make_with(nthreads);
  threadpool_work_t w = {inc, NULL, NULL};
  reset_counter();
  for (int i = 0; i < work; i++) {
    threadpool_add(tp, w);
  }
  threadpool_start(tp);
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  threadpool_destroy(tp);
  wait_for(work);
  return 0;
}

int test_validate_parallelism() {
  reset_counter();
  int nthreads = 5;
  threadpool_work_t w = make_block_at_n(nthreads);
  threadpool_t tp = make_with(nthreads);
  for (int i = 0; i < nthreads; i++) {
    threadpool_add(tp, w);
  }
  threadpool_start(tp);
  // drain will only complete when all callbacks are called in parallel.
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  threadpool_destroy(tp);
  // validate that we indeed called fn+cb for each of the nthreads.
  wait_for(nthreads * 2);
  destroy_block_at_n(w);
  return 0;
}

int test_validate_many_parallelism() {
  reset_counter();
  int nthreads = 1000;
  threadpool_work_t w = make_block_at_n(nthreads);
  threadpool_t tp = make_with(nthreads);
  for (int i = 0; i < nthreads; i++) {
    threadpool_add(tp, w);
  }
  threadpool_start(tp);
  // drain will only complete when all callbacks are called in parallel.
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  threadpool_destroy(tp);
  // validate that we indeed called fn+cb for each of the nthreads.
  wait_for(nthreads * 2);
  destroy_block_at_n(w);
  return 0;
}

int test_pause() {
  int nthreads = 2;
  // this test is time-dependent, since we try to stop the pool immediately
  // after starting. ideally we would inject a way to delay the pool from
  // starting work.
  int work = nthreads * 10000;
  threadpool_t tp = make_with(nthreads);
  threadpool_work_t w = {inc, NULL, NULL};
  reset_counter();
  for (int i = 0; i < work; i++) {
    threadpool_add(tp, w);
  }
  threadpool_start(tp);
  // wait for current work to finish, but don't take up additional work.
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_destroy(tp);
  EXPECT_LONG_GT((long)work, get_counter());
  return 0;
}

int test_pause_resume() {
  int nthreads = 2;
  // this test is time-dependent, since we try to stop the pool immediately
  // after starting. ideally we would inject a way to delay the pool from
  // starting work.
  int work = nthreads * 10000;
  threadpool_t tp = make_with(nthreads);
  threadpool_work_t w = {inc, NULL, NULL};
  reset_counter();
  for (int i = 0; i < work; i++) {
    threadpool_add(tp, w);
  }
  threadpool_start(tp);
  // wait for current work to finish, but don't take up additional work.
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  long mid = get_counter();
  EXPECT_LONG_GT((long)work, mid);
  // restart the pool and stop it shortly afterward -- again this going to be
  // flaky because of timing issues.
  threadpool_start(tp);
  xsleep();
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  threadpool_destroy(tp);
  long final = get_counter();
  EXPECT_LONG_LT(mid, final);
  EXPECT_LONG_GT((long)work, final);
  return 0;
}

int test_drain_resume() {
  int nthreads = 5;
  int work = nthreads * 100;
  threadpool_work_t w = {inc, NULL, NULL};
  threadpool_t tp = make_with(nthreads);
  // add work, drain, add more.
  reset_counter();
  threadpool_start(tp);
  for (int i = 0; i < work; i++) {
    threadpool_add(tp, w);
  }
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  wait_for(work);

  for (int i = 0; i < work; i++) {
    threadpool_add(tp, w);
  }
  threadpool_start(tp);
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  wait_for(2 * work);

  threadpool_destroy(tp);
  return 0;
}

int test_pause_queue_resume() {
  int nthreads = 5;
  int work = nthreads * 100;
  threadpool_work_t w = {inc, NULL, NULL};
  threadpool_t tp = make_with(nthreads);
  // add work, stop, add more.
  reset_counter();
  threadpool_start(tp);
  for (int i = 0; i < work; i++) {
    threadpool_add(tp, w);
  }
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  long mid = get_counter();
  EXPECT_LONG_GTE((long)work, mid);

  for (int i = 0; i < work; i++) {
    threadpool_add(tp, w);
  }
  threadpool_start(tp);
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  wait_for(2 * work);

  threadpool_destroy(tp);
  return 0;
}

struct add_work_arg {
  threadpool_work_t w;
  threadpool_t p;
  int n;
};
void *add_work(void *w) {
  struct add_work_arg *a = w;
  for (int i = 0; i < a->n; i++) {
    threadpool_add(a->p, a->w);
  }
  return NULL;
}

int test_drain_queue_resume() {
  // this test is time-dependent, since we try to stop the pool immediately
  // after starting. ideally we would inject a way to delay the pool from
  // starting work.
  int nthreads = 5;
  int work = nthreads * 100;
  threadpool_t tp = make_with(nthreads);

  threadpool_work_t w = {inc, NULL, NULL};
  struct add_work_arg arg = {w, tp, work};

  // add work, drain and add more while draining.
  reset_counter();
  threadpool_start(tp);
  for (int i = 0; i < work; i++) {
    threadpool_add(tp, w);
  }
  pthread_t tid;
  pthread_create(&tid, NULL, add_work, &arg); // add work in the background.
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  pthread_join(tid, NULL);
  wait_for_gt(work); // we might have slipped new work in before the drain.
  threadpool_start(tp);
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  wait_for(2 * work);

  threadpool_destroy(tp);
  return 0;
}

int test_pause_queue_destroy() {
  threadpool_work_t w = {inc, NULL, NULL};
  threadpool_t tp = make_with(5);
  // add work, stop, add more.
  reset_counter();
  threadpool_start(tp);
  for (int i = 0; i < 1000; i++) {
    threadpool_add(tp, w);
  }
  threadpool_stop(tp, THREADPOOL_STOP_WAIT);
  long mid = get_counter();
  // add more, but don't run them.
  for (int i = 0; i < 1000; i++) {
    threadpool_add(tp, w);
  }
  // destroy immediately
  threadpool_destroy(tp);
  // work shouldn've been executed.
  EXPECT_LONG_EQ(mid, get_counter());
  return 0;
}

int main() {
  ADD_TEST(test_create);
  ADD_TEST(test_run_one);
  ADD_TEST(test_double_start);
  ADD_TEST(test_double_stop);
  ADD_TEST(test_run_many);
  ADD_TEST(test_counters);
  ADD_TEST(test_queue_work);
  ADD_TEST(test_many_threads);
  ADD_TEST(test_drain);
  ADD_TEST(test_validate_parallelism);
  ADD_TEST(test_validate_many_parallelism);
  ADD_TEST(test_pause);
  ADD_TEST(test_pause_resume);
  ADD_TEST(test_pause_queue_resume);
  ADD_TEST(test_drain_resume);
  ADD_TEST(test_drain_queue_resume);
  ADD_TEST(test_pause_queue_destroy);
  run_tests(/*fail_fast=*/0);
  return 0;
}
