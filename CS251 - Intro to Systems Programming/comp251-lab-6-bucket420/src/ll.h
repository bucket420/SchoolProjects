#ifndef __LL_H__
#define __LL_H__

#include "thread_pool.h"

// Linked list of work. Not thread-safe.
struct ll {
  int n; // count of work
  threadpool_work_t work;
  struct ll *next;
  struct ll *tail; // head pointer is guaranteed to point to tail
};

// Add an element to the linked list, returning the new list.
struct ll *ll_add(struct ll *h, threadpool_work_t work);

// Take a single value from the linked list's tail, populating work and
// returning the new list.
struct ll *ll_take(struct ll *h, threadpool_work_t *work);

// Join two linked lists, returning a pointer to the new list.
struct ll *ll_join(struct ll *h0, struct ll *h1);

// Free the linked list.
void ll_free(struct ll *h);

// Return the number of elemnts in the linked list.
int ll_poll(struct ll *h);

#endif
