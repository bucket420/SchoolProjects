#include "ll.h"

#include <stdlib.h>

#include "thread_pool.h"
#include "util.h"

struct ll *ll_add(struct ll *h, threadpool_work_t work) {
  // add at head
  struct ll *n = malloc(sizeof(struct ll));
  n->work = work;
  n->tail = n;
  n->next = NULL;
  n->n = 1;
  if (!h) {
    return n;
  }

  n->n = h->n + 1;
  h->next = n;
  n->tail = h->tail;
  return n;
}

struct ll *ll_take(struct ll *h, threadpool_work_t *work) {
  // take at tail
  if (!h) {
    return NULL;
  }

  struct ll *tail = h->tail;
  *work = tail->work;
  if (!tail->next) {
    assert(tail == h);
    free(tail);
    return NULL;
  }
  h->tail = tail->next;
  h->n -= 1;
  free(tail);
  return h;
}

struct ll *ll_join(struct ll *h0, struct ll *h1) {
  if (!h0) {
    return h1;
  }

  if (!h1) {
    return h0;
  }

  h0->next = h1->tail;
  h1->tail = h0->tail;
  h1->n += h0->n;
  return h1;
}

void ll_free(struct ll *h) {
  if (!h) {
    return;
  }
  struct ll *c = h->tail;
  while (c) {
    h = c->next;
    free(c);
    c = h;
  }
}

int ll_poll(struct ll *h) {
  int r = 0;
  if (h) {
    r = h->n;
  }
  return r;
}
