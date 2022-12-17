#include <stdlib.h>
#include <string.h>

#include "../ll.h"
#include "../thread_pool.h"
#include "test_utils.h"

threadpool_work_t make(long n) {
  threadpool_work_t work;
  memset(&work, 0, sizeof(threadpool_work_t));
  work.work = (void *)n;
  return work;
}

long is(threadpool_work_t work) { return (long)work.work; }

int test_add_take() {
  threadpool_work_t work = make(0);
  struct ll *ll = NULL;
  ll = ll_add(ll, make(42));
  EXPECT_NOTNULL(ll);
  ll = ll_take(ll, &work);
  EXPECT_NULL(ll);
  EXPECT_LONG_EQ(42l, is(work));
  return 0;
}

int test_add_add_take() {
  threadpool_work_t work = make(0);
  struct ll *ll = NULL;
  ll = ll_add(ll, make(42));
  ll = ll_add(ll, make(43));
  EXPECT_NOTNULL(ll);
  ll = ll_take(ll, &work);
  EXPECT_NOTNULL(ll);
  EXPECT_LONG_EQ(42l, is(work));
  ll = ll_take(ll, &work); // clean up
  return 0;
}

int test_several() {
  threadpool_work_t work = make(0);
  struct ll *ll = NULL;
  for (int i = 1; i <= 10; i++) {
    ll = ll_add(ll, make(i));
  }
  EXPECT_NOTNULL(ll);
  for (int i = 1; i <= 10; i++) {
    ll = ll_take(ll, &work);
    EXPECT_LONG_EQ((long)i, is(work));
  }
  return 0;
}

int test_alternate_add_take() {
  threadpool_work_t work = make(0);
  struct ll *ll = NULL;

  ll = ll_add(ll, make(1));
  ll = ll_add(ll, make(2));
  // ll = 1, 2

  ll = ll_take(ll, &work);
  EXPECT_LONG_EQ(1l, is(work));

  ll = ll_add(ll, make(3));
  ll = ll_add(ll, make(4));
  // ll = 2, 3, 4

  ll = ll_take(ll, &work);
  EXPECT_LONG_EQ(2l, is(work));
  ll = ll_take(ll, &work);
  EXPECT_LONG_EQ(3l, is(work));

  ll = ll_add(ll, make(5));
  // ll = 4, 5

  ll = ll_take(ll, &work);
  EXPECT_LONG_EQ(4l, is(work));
  ll = ll_take(ll, &work);
  EXPECT_LONG_EQ(5l, is(work));

  return 0;
}

int test_take_null() {
  threadpool_work_t work = make(0);
  struct ll *ll = NULL;
  ll = ll_take(ll, &work);
  EXPECT_NULL(ll);
  return 0;
}

int test_add_take_take_empty() {
  threadpool_work_t work = make(0);
  struct ll *ll = NULL;
  ll = ll_add(ll, make(42));
  ll = ll_take(ll, &work);
  ll = ll_take(ll, &work);
  EXPECT_NULL(ll);
  return 0;
}

struct ll *make_chain(int start, int n) {
  struct ll *ll = NULL;
  for (int i = start; i < start + n; i++) {
    ll = ll_add(ll, make(i));
  }
  return ll;
}

int test_join_empty() {
  struct ll *left, *right;
  left = right = NULL;
  struct ll *r = ll_join(left, right);
  EXPECT_NULL(r);
  return 0;
}

int test_join_lempty() {
  struct ll *left, *right;
  left = right = NULL;
  right = make_chain(1, 5);
  struct ll *r = ll_join(left, right);
  threadpool_work_t work = make(0);
  for (int i = 1; i < 6; i++) {
    r = ll_take(r, &work);
    EXPECT_LONG_EQ((long)i, is(work));
  }
  EXPECT_NULL(r);
  return 0;
}

int test_join_rempty() {
  struct ll *left, *right;
  left = right = NULL;
  right = make_chain(1, 5);
  struct ll *r = ll_join(left, right);
  threadpool_work_t work = make(0);
  for (int i = 1; i < 6; i++) {
    r = ll_take(r, &work);
    EXPECT_LONG_EQ((long)i, is(work));
  }
  EXPECT_NULL(r);
  return 0;
}

int test_join() {
  struct ll *left, *right;
  left = right = NULL;
  left = make_chain(1, 5);
  right = make_chain(10, 5);
  struct ll *r = ll_join(left, right);
  threadpool_work_t work = make(0);
  for (int i = 1; i < 6; i++) {
    r = ll_take(r, &work);
    EXPECT_LONG_EQ((long)i, is(work));
  }
  for (int i = 10; i < 15; i++) {
    r = ll_take(r, &work);
    EXPECT_LONG_EQ((long)i, is(work));
  }
  EXPECT_NULL(r);
  return 0;
}

int test_free() {
  struct ll *l = make_chain(1, 100);
  ll_free(l);
  return 0;
}

int test_counters() {
  struct ll *ll = make_chain(1, 5);
  threadpool_work_t work;
  for (int i = 1; i < 6; i++) {
    EXPECT_INT_EQ(6 - i, ll_poll(ll));
    ll = ll_take(ll, &work);
  }
  EXPECT_INT_EQ(0, ll_poll(ll));
  return 0;
}

int test_counters_join_right_heavy() {
  struct ll *left, *right;
  left = right = NULL;
  left = make_chain(0, 15);
  right = make_chain(0, 5);
  struct ll *r = ll_join(left, right);
  threadpool_work_t work = make(0);
  for (int i = 0; i < 20; i++) {
    EXPECT_INT_EQ(20 - i, ll_poll(r));
    r = ll_take(r, &work);
  }
  EXPECT_INT_EQ(0, ll_poll(r));
  EXPECT_NULL(r);
  return 0;
}

int test_counters_join_left_heavy() {
  struct ll *left, *right;
  left = right = NULL;
  left = make_chain(0, 5);
  right = make_chain(0, 15);
  struct ll *r = ll_join(left, right);
  threadpool_work_t work = make(0);
  for (int i = 0; i < 20; i++) {
    EXPECT_INT_EQ(20 - i, ll_poll(r));
    r = ll_take(r, &work);
  }
  EXPECT_INT_EQ(0, ll_poll(r));
  EXPECT_NULL(r);
  return 0;
}

int main() {
  ADD_TEST(test_add_take);
  ADD_TEST(test_add_add_take);
  ADD_TEST(test_several);
  ADD_TEST(test_take_null);
  ADD_TEST(test_add_take_take_empty);
  ADD_TEST(test_alternate_add_take);
  ADD_TEST(test_join_empty);
  ADD_TEST(test_join_lempty);
  ADD_TEST(test_join_rempty);
  ADD_TEST(test_join);
  ADD_TEST(test_free);
  ADD_TEST(test_counters);
  ADD_TEST(test_counters_join_left_heavy);
  ADD_TEST(test_counters_join_right_heavy);
  run_tests(/*fail_fast=*/0);
}
