// Word Count. Tests map insert, replace, and resize operations.
//
// Counts words in the file specified with -f.

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "map.h"
#include "rand1.h"

#define MAX_TOKEN_LEN 1024
#define MAP_SIZE 1024

int resize_count = 0;

void usage() {
  printf("Usage: word_count -f <file> [-s <size>] [-d] [-r]\n"
         "Options:\n"
         "\t-f <file>  Count words in <file>\n"
         "\t-s <size>  Size of map\n"
         "\t-d         Print map debug stats\n"
         "\t-r         Periodically resize between 1 and 8k entries (stress "
         "test)\n");
}

// Convert a string to lower case and strip whitespace and punctuation.
//
// Resizes string by nulling the unused suffix.
void lower_and_strip(char *str) {
  char *t = str;
  char *c = str;
  for (; *c; c++) {
    if (isalnum(*c)) {
      assert(t <= c);
      *t = tolower(*c); // t is lte c
      t++;
    }
  }
  // c is null terminator; cut end of string after alnum chars.
  while (t < c) {
    *(t++) = '\0';
  }
}

void maybe_resize(map_t *map) {
  // resize every 100 calls (ish)
  if (rand1() < 0.1) {
    // choose random size 1 - 8k
    uint32_t size = (uint32_t)(rand1() * (1 << 13)) + 1;
    map_resize(map, size);
    resize_count += 1;
    // if you want to compute peak max depth, you could keep track of it here by
    // calling map_metrics and saving it in a global (like resize_count).
  }
}

// Run word count on the file, saving results to m.
//
// Here we abuse the void* to treat it as a uint64_t. Not portable!
void word_count(FILE *fd, map_t *m, int do_resize) {
  char buff[MAX_TOKEN_LEN];
  while (fscanf(fd, "%s", buff) != EOF) {
    // read one word and normalize it.
    lower_and_strip(buff);
    // if there's no remaining string after normalizing, skip.
    if (!strlen(buff)) {
      continue;
    }

    // insert or update, keeping track of whether the key exists. validates
    // invariants with asserts.
    void *ptr;
    int exists_get = map_get(m, buff, &ptr);

    // exists_get -> ptr
    assert(!exists_get || ptr);
    // ptr -> exists_get
    assert(!ptr || exists_get);

    // if value is new, ptr should be NULL (0). otherwise it will be the last
    // value.
    uint64_t val = (uint64_t)ptr;
    // increment and store back.
    val++;
    int new_put = map_put(m, buff, (void *)val);

    // exists_get -> !new_put
    assert(!exists_get || !new_put);
    // try resize
    if (do_resize) {
      maybe_resize(m);
    }
  }
}

// Apply function to print word counts.
void *print_result(const char *word, void *count) {
  printf("%-*lu %s\n", 10, (uint64_t)count, word);
  return count; // do not modify value
}

int main(int argc, char **argv) {
  char fname[MAX_TOKEN_LEN];
  int debug = 0;
  int resize = 0;
  uint64_t size = MAP_SIZE;
  char c;
  fname[0] = '\0';

  // parse args and validate.
  while ((c = getopt(argc, argv, "rdf:s:")) != EOF) {
    switch (c) {
    case 'f':
      strncpy(fname, optarg, MAX_TOKEN_LEN);
      break;
    case 'd':
      debug = 1;
      break;
    case 'r':
      resize = 1;
      srand(time(NULL));
      break;
    case 's':
      errno = 0;
      size = strtol(optarg, NULL, 10);
      if (errno) {
        perror("strtol");
        exit(1);
      } else if (size <= 0) {
        printf("-s <size>; size must be nonnegative: %lu\n", size);
        exit(1);
      }
      break;
    default:
      usage();
      exit(1);
    }
  }
  if (!strlen(fname)) {
    usage();
    exit(1);
  }

  // run the word count on the specified file.
  map_t *map = map_create(size);
  FILE *f = fopen(fname, "r");
  assert(f);
  word_count(f, map, resize);
  fclose(f);

  // print word count results
  map_apply(map, print_result);

  // print debug info.
  if (debug) {
    printf("\n\n\n----------------------------------\n"
           "Map Stats:\n"
           "----------------------------------\n");
    map_metrics_t *metrics = map_metrics(map);
    printf("num_entries:\t%d\n", metrics->num_entries);
    printf("max_depth:\t%d\n", metrics->max_depth);
    printf("size:\t\t%d\n", metrics->curr_size);
    free(metrics);
  }
  if (resize) {
    printf("resize count:\t%d\n", resize_count);
  }

  // clean up!
  map_free(&map);
}
