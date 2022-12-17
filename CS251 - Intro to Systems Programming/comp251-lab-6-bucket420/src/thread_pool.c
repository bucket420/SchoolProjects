#include "thread_pool.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

#include "ll.h"
#include "util.h"

// Threadpool implementation.
struct threadpool {
  enum tp_state state;        // current state
  threadpool_config_t config; // copy of the thread pool config
  pthread_mutex_t mu;         // mutex guarding elements
  pthread_cond_t avail; // condition variable; worker threads wait on this cond
                        //  until work is available
  pthread_t *threads;   // pointer to array of worker threads
  struct targ *targs;   // worker thread arguments
  threadpool_counters_t counters; // status counters
  struct ll *queued_work;         // list of queued work
  struct ll *paused_work;         // list of work added while STOPPING/DRAINING
};

// Per-worker thread argument.
struct targ {
  threadpool_t pool; // the thread pool the worker belongs to
  int id;            // the worker's id
};

// Returns whether a thread should halt.
//
// The thread should halt when either of the two conditions are met:
//
// - The pool is STOPPING, or
// - The pool is DRAINING and queued_work is empty.
//
// Requires pool->mu to be held by the caller and therefore can read shared
// state without locking the mutex.
int is_stop(threadpool_t pool) {
  return pool->state == THREADPOOL_STATE_STOPPING ||
         (pool->state == THREADPOOL_STATE_DRAINING &&
          !ll_poll(pool->queued_work));
}

// Get a work item.
//
// Accepts a thread pool and a pointer to a work item. Attempts to find
// work from the pool. Blocks until either:
//
// - Work is available in the work queue. In this case, the work is copied to
//   the work paramter and the function returns 1.
// - The thread should halt. In this case, the work is not filled in and the
//   function returns 0.
//
// This function should not be called while the pool's mutex is held, as it will
// acquire the mutex in order to check the state of the work queue, dequeue
// elements, wait on the avail condition variable, etc.
//
// Updates the pool's counters.queued_work counter when work is dequeued.
int get_work(threadpool_t pool, threadpool_work_t *work) {
  pthread_mutex_lock(&pool->mu);
  while (!is_stop(pool) && !ll_poll(pool->queued_work)) {
    pthread_cond_wait(&pool->avail, &pool->mu);
  }
  if (is_stop(pool)) {
    pthread_mutex_unlock(&pool->mu);
    return 0;
  }
  pool->queued_work = ll_take(pool->queued_work, work);
  pool->counters.queued_work--;
  pthread_mutex_unlock(&pool->mu);
  return 1;
}

void *tp_worker(void *work) {
  struct targ *arg = work;
  threadpool_work_t w;
  // Run forever, until get_work returns 0.
  while (1) {
    if (!get_work(arg->pool, &w)) {
      // When get_work returns 0, the thread pool should halt.
      return NULL;
    }
    // get_work returned 1, so w should be new work.

    // Do the work and call the callback with the returned value (if the
    // callback is non-null).
    void *work_result = w.fn(w.work);
    if (w.cb) {
      w.cb(work_result);
    }
    // update counters; protected by the mutex.
    pthread_mutex_lock(&arg->pool->mu);
    arg->pool->counters.completed_work++;
    pthread_mutex_unlock(&arg->pool->mu);
  }
  return NULL;
}

threadpool_t threadpool_create(threadpool_config_t config) {
  struct threadpool *p = malloc(sizeof(struct threadpool));
  p->config = config;

  // initialize mutex and condition variable.
  assertz(pthread_mutex_init(&p->mu, NULL));
  assertz(pthread_cond_init(&p->avail, NULL));

  p->threads = calloc(config.nthreads, sizeof(pthread_t));
  p->targs = calloc(config.nthreads, sizeof(struct targ));
  p->queued_work = NULL;
  p->paused_work = NULL;
  // initial state is stopped.
  p->state = THREADPOOL_STATE_STOPPED;
  memset(&p->counters, 0, sizeof(threadpool_counters_t));

  return p;
}

int threadpool_start(threadpool_t tp) {
  pthread_mutex_lock(&tp->mu);
  if (tp->state != THREADPOOL_STATE_STOPPED) {
    // already running, return.
    pthread_mutex_unlock(&tp->mu);
    return tp->state;
  }

  // join work queues
  if (tp->paused_work) {
    tp->queued_work = ll_join(tp->queued_work, tp->paused_work);
    tp->paused_work = NULL; // clear paused work
  }

  // start worker threads
  for (int i = 0; i < tp->config.nthreads; i++) {
    struct targ *arg = &tp->targs[i];
    arg->pool = tp;
    arg->id = i;
    assertz(pthread_create(&tp->threads[i], NULL, tp_worker, arg));
  }
  DEBUG_PRINT("worker threads started...\n");

  tp->state = THREADPOOL_STATE_RUNNING;
  pthread_mutex_unlock(&tp->mu);
  return THREADPOOL_STATE_RUNNING;
}

void threadpool_add(threadpool_t tp, threadpool_work_t work) {
  assert(work.fn);
  pthread_mutex_lock(&tp->mu);
  if (tp->state != THREADPOOL_STATE_RUNNING) {
    DEBUG_PRINT("queueing work in secondary queue...\n");
    tp->paused_work = ll_add(tp->paused_work, work);
  } else {
    DEBUG_PRINT("adding work...\n");
    tp->queued_work = ll_add(tp->queued_work, work);
  }
  tp->counters.queued_work++;
  // if there is any thread waiting, wake them.
  // TODO: depending on your implementation, this may need to be
  // pthread_cond_signal or pthread_cond_broadcast
  pthread_cond_broadcast(&tp->avail);
  pthread_mutex_unlock(&tp->mu);
}

void threadpool_destroy(threadpool_t tp) {
  // pool must be stopped and worker threads joined.
  assert(tp->state == THREADPOOL_STATE_STOPPED);
  // free the pool.
  ll_free(tp->queued_work);
  ll_free(tp->paused_work);
  free(tp->threads);
  free(tp->targs);
  free(tp);
}

int threadpool_stop(threadpool_t tp, int options) {
  pthread_mutex_lock(&tp->mu);
  if (tp->state == THREADPOOL_STATE_STOPPED) {
    // If already stopped, return.
    pthread_mutex_unlock(&tp->mu);
    return 0;
  }
  if (options & THREADPOOL_STOP_WAIT) {
    tp->state = THREADPOOL_STATE_STOPPING;
  } else {
    tp->state = THREADPOOL_STATE_DRAINING;
  }
  // wake any waiting threads
  pthread_cond_broadcast(&tp->avail);
  pthread_mutex_unlock(&tp->mu);
  for (int i = 0; i < tp->config.nthreads; i++) {
    pthread_join(tp->threads[i], NULL);
  }
  pthread_mutex_lock(&tp->mu);
  DEBUG_PRINT("add threads stopped, state = STOPPED\n");
  tp->state = THREADPOOL_STATE_STOPPED;
  pthread_mutex_unlock(&tp->mu);
  return 0;
}

threadpool_counters_t threadpool_counters(threadpool_t tp) {
  // return the current counters
  pthread_mutex_lock(&tp->mu);
  threadpool_counters_t c = tp->counters;
  pthread_mutex_unlock(&tp->mu);
  return c;
}
