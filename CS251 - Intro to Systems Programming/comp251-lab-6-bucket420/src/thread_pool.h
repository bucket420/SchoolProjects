#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

// Thread pool library.
//
// A thread pool is a collection of work threads that take work items from a
// work queue and process these items in parallel. A thread pool runs
// continuously and can service work as it arrives in the pool. The parallelism
// of the thread pool is a parameter to its creation.
//
// Typical usage
// -------------
//
//     threadpool_config_t config;
//     config.nthreads = <...>;
//     threadpool_t tp = threadpool_create(config);
//
//     ...
//
//     threadpool_start(tp);
//
//     ...
//
//     threadpool_work_t my_work;
//     my_work.work = my_work_argument;
//     my_work.fn = my_work_function;
//     my_work.cb = my_work_callback;  // to receive work result
//
//     threadpool_add(tp, work);
//
//     ...
//
//     threadpool_stop(tp, THREADPOOL_STOP_WAIT);
//     threadpool_destroy(tp);
//
//
// Starting + stopping thread pools
// -------------------------------
//
// Thread pools can be in one of four states. The transitions between states are
// controlled by the user of the thread pool by calling threadpool_start and
// threadpool_stop.
//
// Note that threadpool_stop is a blocking function -- it only returns when all
// work has been stopped.
//
// - stopped:  the thread pool is not doing work or waiting for work. work
//             added in this state will be queued until the thread pool is
//             started.
//
// - running:  the thread pool is either doing work or waiting for work. work
//             added in this state will be run immediately if there are any idle
//             threads, or will be queued until a thread becomes idle.
//             thread pools enter the running state when threadpool_start is
//             called.
//
// - stopping: the thread pool may have active threads finishing work, but will
//             not start new work from the queue. work that is added in this
//             state will be queued until the next time the thread pool is
//             started. when all active threads finish their work, the
//             thread pool will transition to the stopped state. a thread pool
//             enters the stopping state when threadpool_stop is called with the
//             parameter THREADPOOL_STOP_WAIT.
//
// - draining: the thread pool will finish all currently queued work and then
//             transition to the stopped state. work queued while a thread pool
//             is draining is added to a secondary queue, and will not be
//             processed until the thread pool is restarted. a thread pool
//             enters the draining state when threadpool_stop is called with the
//             parameter THREADPOOL_STOP_DRAIN.
//
// Thread pool work items and functions
// -----------------------------------
//
// The type of thread pool work items is threadpool_work_t. This struct has
// three fields:
//
// - work: a unit of work, in the form of a pointer. this pointer will be
//         supplied as a paramter to the work function, and can be used to
//         represent the arguments for an instance of the function to run. work
//         may be NULL, if there are no desired arguments for the work function.
//
// - fn:   the function to run. each work item may have a pointer to a different
//         function or they may be the same. this allows for heterogenous pools
//         of work. the function should return void* (the work result) and take
//         a parameter of void* (the work argument). fn must not be NULL.
//
// - cb:   a callback function to run when work is complete. this function is
//         called with the return value of fn. the function should return
//         nothing and accept a single void* parameter. the callback may be
//         NULL. in most cases the callback is unnecessary and the typical use
//         case will make cb NULL.

// Thread pool states. See above.
enum tp_state {
  THREADPOOL_STATE_STOPPED,
  THREADPOOL_STATE_STOPPING,
  THREADPOOL_STATE_DRAINING,
  THREADPOOL_STATE_RUNNING,
};

// A thread pool.
typedef struct threadpool *threadpool_t;

// Thread pool construction parameters.
typedef struct threadpool_config_t {
  unsigned int nthreads; // number of worker threads to create
} threadpool_config_t;

// Work function signature.
typedef void *(*threadpool_fn)(void *);
// Callback signature.
typedef void (*threadpool_cb)(void *);

typedef struct threadpool_work_t {
  threadpool_fn fn; // work function to be called with work, must not be NULL
  void *work;       // pointer to work, may be NULL
  threadpool_cb cb; // callback function, may be NULL
} threadpool_work_t;

// Counters for retrieving thread pool stats.
typedef struct threadpool_counters_t {
  unsigned int completed_work;
  unsigned int queued_work;
} threadpool_counters_t;

// Stop a thread pool by completing all in-flight work.
#define THREADPOOL_STOP_WAIT 1
// Stop a thread pool by completing all queued work.
#define THREADPOOL_STOP_DRAIN 0

// Create a thread pool.
//
// Thread pools are initially STOPPED.
threadpool_t threadpool_create(threadpool_config_t config);

// Destroy a thread pool.
//
// A thread pool must be in the STOPPED state in order to destroy it.
//
// After this funciton returns, the tp argument should no longer be used and
// does not need to be freed.
void threadpool_destroy(threadpool_t tp);

// Start a thread pool if it is STOPPED.
//
// Returns the state of the pool (see above).
//
// If the thread pool is STOPPING, DRAINING, or RUNNING, this call has no
// effect.
//
// Transitions the thread pool to the RUNNING state.
int threadpool_start(threadpool_t tp);

// Stop a thread pool.
//
// The thread pool can be stopped with THREADPOOL_STOP_WAIT or
// THREADPOOL_STOP_DRAIN. In the former case, the thread pool transitions to the
// STOPPING state and all in-flight work is completed. No new work is started.
// in the latter, the thread pool is allowed to complete all queued work.
//
// This function blocks and only returns when the thread pool is STOPPED.
int threadpool_stop(threadpool_t tp, int options);

// Add work to a thread pool.
//
// Work added to a STOPPED thread pool is queued until the thread pool is
// RUNNING. After a thread pool becomes RUNNING, work is queued until a worker
// thread becomes free and can process the work.
//
// Work added to a DRAINING or STOPPING thread pool is queued until the
// thread pool stops and then is later restarted.
void threadpool_add(threadpool_t tp, threadpool_work_t work);

// Returns a snapshot of the current counter values.
threadpool_counters_t threadpool_counters(threadpool_t tp);

#endif
