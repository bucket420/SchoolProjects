#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <stdint.h>
#include <stdio.h>

#define TEST_NAME_LEN 80
struct test_case {
  char name[TEST_NAME_LEN];
  char suite[TEST_NAME_LEN];
  int (*test)();
};

void add_test(struct test_case test);
void run_tests();

// returns the number of pages allocated to the process.
uint64_t total_mem_alloc();

#define ADD_TEST(fn)                                                           \
  do {                                                                         \
    struct test_case tc;                                                       \
    tc.test = fn;                                                              \
    snprintf(tc.suite, TEST_NAME_LEN, "%s", __FILE__);                         \
    snprintf(tc.name, TEST_NAME_LEN, "%s", "" #fn "");                         \
    add_test(tc);                                                              \
  } while (0);

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Test expectation macros

#define EXPECT(expr, message)                                                  \
  do {                                                                         \
    if ((expr)) {                                                              \
    } else {                                                                   \
      printf("[%s:%d] Assertion %s failed: %s\n", __FILE__, __LINE__,          \
             "" #expr "", message);                                            \
      return 1;                                                                \
    }                                                                          \
  } while (0);

#define EXPECT_TRUE(expr) EXPECT(expr, "" #expr "")
#define EXPECT_FALSE(expr) EXPECT(!(expr), "!" #expr "")

#define EXPECT_OPERATOR(val0, val1, OP, fmt)                                   \
  do {                                                                         \
    if ((val0)OP(val1)) {                                                      \
    } else {                                                                   \
      fprintf(stderr,                                                          \
              "[%s:%d] Assertion %s failed (expected: " fmt "; actual: " fmt   \
              ")\n",                                                           \
              __FILE__, __LINE__, "" #val0 " " #OP " " #val1 "", val0, val1);  \
      return 1;                                                                \
    }                                                                          \
  } while (0);

#define EXPECT_INT_LT(val0, val1) EXPECT_OPERATOR(val0, val1, <, "%d")
#define EXPECT_INT_LTE(val0, val1) EXPECT_OPERATOR(val0, val1, <=, "%d")
#define EXPECT_INT_GT(val0, val1) EXPECT_OPERATOR(val0, val1, >, "%d")
#define EXPECT_INT_GTE(val0, val1) EXPECT_OPERATOR(val0, val1, >=, "%d")
#define EXPECT_INT_EQ(val0, val1) EXPECT_OPERATOR(val0, val1, ==, "%d")
#define EXPECT_INT_NEQ(val0, val) EXPECT_OPERATOR(val0, val1, !=, "%d")

#define EXPECT_UINT_LT(val0, val1) EXPECT_OPERATOR(val0, val1, <, "%u")
#define EXPECT_UINT_LTE(val0, val1) EXPECT_OPERATOR(val0, val1, <=, "%u")
#define EXPECT_UINT_GT(val0, val1) EXPECT_OPERATOR(val0, val1, >, "%u")
#define EXPECT_UINT_GTE(val0, val1) EXPECT_OPERATOR(val0, val1, >=, "%u")
#define EXPECT_UINT_EQ(val0, val1) EXPECT_OPERATOR(val0, val1, ==, "%u")
#define EXPECT_UINT_NEQ(val0, val) EXPECT_OPERATOR(val0, val1, !=, "%u")

#define EXPECT_LONG_LT(val0, val1) EXPECT_OPERATOR(val0, val1, <, "%ld")
#define EXPECT_LONG_LTE(val0, val1) EXPECT_OPERATOR(val0, val1, <=, "%ld")
#define EXPECT_LONG_GT(val0, val1) EXPECT_OPERATOR(val0, val1, >, "%ld")
#define EXPECT_LONG_GTE(val0, val1) EXPECT_OPERATOR(val0, val1, >=, "%ld")
#define EXPECT_LONG_EQ(val0, val1) EXPECT_OPERATOR(val0, val1, ==, "%ld")
#define EXPECT_LONG_NEQ(val0, val) EXPECT_OPERATOR(val0, val1, !=, "%ld")

#define EXPECT_ULONG_LT(val0, val1) EXPECT_OPERATOR(val0, val1, <, "%lu")
#define EXPECT_ULONG_LTE(val0, val1) EXPECT_OPERATOR(val0, val1, <=, "%lu")
#define EXPECT_ULONG_GT(val0, val1) EXPECT_OPERATOR(val0, val1, >, "%lu")
#define EXPECT_ULONG_GTE(val0, val1) EXPECT_OPERATOR(val0, val1, >=, "%lu")
#define EXPECT_ULONG_EQ(val0, val1) EXPECT_OPERATOR(val0, val1, ==, "%lu")
#define EXPECT_ULONG_NEQ(val0, val) EXPECT_OPERATOR(val0, val1, !=, "%lu")

#define EXPECT_NULL(val0) EXPECT_FALSE(val0)
#define EXPECT_NOTNULL(val0) EXPECT_TRUE(val0)

#endif
