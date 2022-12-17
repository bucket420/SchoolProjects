#include "ii.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../map/map.h"
#include "../thread_pool.h"
#include "../util.h"

#define MAX_LINE_LEN 1024
#define PER_FILE_MAP_SIZE 1024

char *TEXT_EXTENSIONS[] = {".txt", NULL};

map_t *ii = NULL;           // the ii
map_t *file_aliases = NULL; // filename -> alias

char **filter_list = NULL; // words to filter; only used during call to
                           // build_ii

// convert a string to lower case and strip all non-alphanumeric characters.
int lower_and_strip(char *str) {
  char *t = str;
  char *c = str;
  for (; *c; c++) {
    if (isalnum(*c)) {
      assert(t <= c);
      *t = tolower(*c); // t is lte c
      t++;
    }
  }
  int len = t - str;
  // c is null terminator; cut end of string after alnum chars.
  while (t < c) {
    *(t++) = '\0';
  }
  return len;
}

// entry for a word, may be shared by several threads. relies on thread safe
// implementation of map.
struct word_entry {
  char *word;   // word
  map_t *files; // map of filename->file_entry
};

// entry for a word within a file.
struct file_entry {
  int n;          // number of appearances
  int *v;         // list of ints, the line numbers the word appears on
  const char *fn; // the filename
};

// given a word, either get the existing word entry or create a new word entry
// and initalize the file map for the word.
struct word_entry *put_or_get(const char *word) {
  struct word_entry *z = malloc(sizeof(struct word_entry));
  assert(z);
  z->files = map_create(PER_FILE_MAP_SIZE);
  struct word_entry *old;
  int exists = map_get_or_put(ii, word, (void **)&old, z);
  if (exists) {
    map_free(&z->files);
    free(z);
  } else {
    // first insert copies the word to the entry.
    old->word = malloc(strlen(word) + 1);
    assert(old->word);
    strcpy(old->word, word);
  }
  return old;
}

// load word/file list into ii
void *bulk_load_apply_fn(const char *key, void *value) {
  struct word_entry *e = put_or_get(key);
  struct file_entry *v = value;
  map_put(e->files, v->fn, v);
  return value;
}

// load the per-file file entry into the ii by updating the file map for the
// word.
void bulk_load(const char *file, map_t *fm) {
  map_apply(fm, bulk_load_apply_fn);
}

// create/update the file entry for the file/line for a given word.
void put_entry(const char *file, int line, const char *word, map_t *fm) {
  struct file_entry *values;
  int exists = map_get(fm, word, (void **)&values);
  if (!exists) {
    values = calloc(1, sizeof(struct file_entry));
    values->fn = file;
  }
  values->n++;
  values->v = realloc(values->v, sizeof(int) * values->n);
  assert(values->v);
  values->v[values->n - 1] = line;
  map_put(fm, word, values);
}

int in_filter(const char *word, int len) {
  for (char **w = filter_list; *w; w++) {
    if (strncmp(word, *w, len) == 0) {
      return 1;
    }
  }
  return 0;
}

// process a line from the file, updating the file entry for each word.
void process_line(const char *file, int l_no, char *line, map_t *fm) {
  char *save;
  int len;
  for (char *word = strtok_r(line, " ", &save); word;
       word = strtok_r(NULL, " ", &save)) {
    len = lower_and_strip(word);
    if (!len || (filter_list && in_filter(word, len))) {
      continue;
    }
    put_entry(file, l_no, word, fm);
  }
}

// update the ii for the given file.
void process_file(char *file) {
  char buff[MAX_LINE_LEN];
  printf("> Processing %s...\n", file);
  FILE *f = fopen(file, "r");
  if (!f) {
    perror("fopen");
    exit(1);
  }
  // store a map of word -> lines for this file, then bulk update the global
  // map.
  map_t *fm = map_create(PER_FILE_MAP_SIZE);
  int line = 0;
  while (fgets(buff, MAX_LINE_LEN, f)) {
    process_line(file, line, buff, fm);
    line++;
  }
  fclose(f);
  // bulk load results into ii.
  bulk_load(file, fm);
  map_free(&fm);
  printf("> Processing %s done!\n", file);
}

// helper fn to calculate the length of a null-terminated list.
int len(char **lst) {
  int i = 0;
  for (char **e = lst; *e; e++, i++) {
  }
  return i;
}

// free each file entry in a map file->file_entry
void *free_apply_fn_fm(const char *key, void *value) {
  if (!value) {
    printf("wtf fm %s\n", key);
  }
  struct file_entry *e = value;
  free(e->v);
  // filename is owned by caller. free(e->fn);
  free(e);
  return NULL;
}

// free the per-word entry in the ii
void *free_apply_fn_ii(const char *key, void *value) {
  if (!value) {
    printf("wtf ii %s\n", key);
  }
  struct word_entry *e = value;
  map_apply(e->files, free_apply_fn_fm);
  free(e->word);
  map_free(&e->files);
  free(e);
  return NULL;
}

// free the ii
void free_ii() {
  map_apply(ii, free_apply_fn_ii);
  map_free(&ii);
  map_free(&file_aliases);
}

// match the signature for the threadpool
void *process_file_wrapper(void *file) {
  process_file((char *)file);
  return NULL;
}

// build an inverted index by processing words in parallel
void build_ii(char **files, char **filter, int max_parallelism, int map_size) {
  filter_list = filter;
  // create the ii
  ii = map_create(map_size);
  file_aliases = map_create(map_size);
  int n = len(files);
  // build aliases for files -- assign an integer to each file
  for (long i = 0; i < n; i++) {
    map_put(file_aliases, files[i], (void *)i);
    printf("> Alias for %s: %ld\n", files[i], i);
  }
  printf("Processing %d files with %d threads...\n", n, max_parallelism);
  // Use a threadpool to limit parallelism.
  threadpool_config_t cfg;
  cfg.nthreads = max_parallelism;
  threadpool_t tp = threadpool_create(cfg);
  threadpool_start(tp);

  // Create bulk work -- one work item per file
  threadpool_work_t work;

  work.fn = process_file_wrapper;
  work.cb = NULL; // no callback necessary
  for (int i = 0; i < n; i++) {
    work.work = files[i];
    threadpool_add(tp, work);
  }

  // Drain the pool and wait for work to complete.
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  // Drain should be complete; there should be no pending work.
  threadpool_destroy(tp);
}

// list files in a directory, filtered by a list of extensions. if extensions is
// NULL, lists all files.
char **list_files(char *dir, char **extensions) {
  DIR *d;
  struct dirent *ent;
  int dirlen = strlen(dir);
  errno = 0;
  d = opendir(dir);
  if (!d) {
    perror("opendir");
    exit(1);
  }
  char **flist = NULL;
  int n = 0;
  errno = 0;
  while ((ent = readdir(d)) != NULL) {
    if (ent->d_type != DT_REG) {
      continue;
    }
    if (extensions) {
      int found = 0;
      for (char **ext = extensions; *ext && !found; ext++) {
        char *dot = strchr(ent->d_name, '.');
        found = dot && strcmp(dot, *ext) == 0;
      }
      if (!found) {
        continue;
      }
    }
    n++;
    flist = realloc(flist, n * sizeof(char *));
    flist[n - 1] = malloc(dirlen + strlen(ent->d_name) + 2);
    assert(flist[n - 1]);
    strcpy(flist[n - 1], dir);
    strcat(flist[n - 1], "/");
    strcat(flist[n - 1], ent->d_name);
  }
  closedir(d);
  n++;
  flist = realloc(flist, n * sizeof(char *));
  flist[n - 1] = NULL;
  return flist;
}

// globals for dumping the ii
const char **keys = NULL;
unsigned int n_keys = 0;
unsigned int keys_len = 0;

// apply_fn to create a list of all keys
void *key_aggregate_fn(const char *key, void *value) {
  n_keys++;
  // grow list if necessary
  if (n_keys > keys_len) {
    keys_len = keys_len > 0 ? keys_len * 2 : 256;
    keys = realloc(keys, keys_len * sizeof(char *));
    assert(keys);
  }
  keys[n_keys - 1] = key;
  return value;
}

// match qsort desired compare function signature; code from qsort manual
static int cmpstr(const void *p1, const void *p2) {
  return strcmp(*(char *const *)p1, *(char *const *)p2);
}

// argument for a writer thread
struct writer_thread_arg {
  char *dir;
  unsigned int start;
  unsigned int stride;
  unsigned int max;
  unsigned int id;
  unsigned int shards;
};

// argument for apply fn that writes words to files
struct writer_per_word_apply_arg {
  FILE *f;
};

// write an entry for a given word (a single line)
void *writer_per_word_apply_fn(const char *fname, void *value, void *arg) {
  struct writer_per_word_apply_arg *aa = arg;
  struct file_entry *fe = value;
  // look up filename
  char buff[8];
  void *val;
  assertz(!map_get(file_aliases, fname, &val));
  snprintf(buff, 8, "%ld", (long)val);
  fprintf(aa->f, "%s(", buff);
  for (int i = 0; i < fe->n - 1; i++) {
    fprintf(aa->f, "%d,", fe->v[i]);
  }
  fprintf(aa->f, "%d);", fe->v[fe->n - 1]);
  return value;
}

// return the number of digits in a number with the given base
int digits(unsigned int num, unsigned int base) {
  assert(base);
  int digits = 0;
  while (num > 0) {
    num /= base;
    digits++;
  }
  return digits;
}

// thread to write an output shard
void *writer_thread(void *arg) {
  struct writer_thread_arg *wta = arg;
  const char *start_word = keys[wta->start];
  // the last shard should not go over.
  unsigned int max_idx =
      wta->id == wta->shards - 1 ? wta->max : wta->start + wta->stride;
  const char *end_word = keys[max_idx - 1];
  struct word_entry *we;
  char fname[PATH_MAX];
  char fmt[64];
  // create format string for file name based on max number of digits -- e.g.,
  // 0000_1024
  // 0123_1024
  // 1024_1024
  snprintf(fmt, 64, "%%s/%%0%dd-%%0%dd_%%s-%%s.idx", digits(wta->shards, 10),
           digits(wta->shards, 10));
  snprintf(fname, PATH_MAX, fmt, wta->dir, wta->id, wta->shards, start_word,
           end_word);
  printf("> Writing shard %d...\n", wta->id);
  FILE *f = fopen(fname, "w");
  struct writer_per_word_apply_arg aarg = {f};
  for (int idx = wta->start; idx < max_idx; idx++) {
    // process word, printing one word per line.
    fprintf(f, "%s:", keys[idx]);
    map_get(ii, keys[idx], (void **)&we);
    map_apply_arg(we->files, writer_per_word_apply_fn, &aarg);
    fprintf(f, "\n");
  }
  fclose(f);
  printf("> Writing shard %d done!\n", wta->id);
  return NULL;
}

void *file_alias_index_writer(const char *fname, void *val, void *arg) {
  struct writer_per_word_apply_arg *aarg = arg;
  fprintf(aarg->f, "%s;%ld\n", fname, (long)val);
  return val;
}

void write_index(char *dir, unsigned int shards) {
  char fname[PATH_MAX];
  snprintf(fname, PATH_MAX, "%s/ii-%d.aliases", dir, shards);
  printf("> Writing index %s...\n", fname);
  FILE *f = fopen(fname, "w");
  if (!f) {
    char err[PATH_MAX + 50];
    snprintf(err, PATH_MAX + 50, "error opening %s", fname);
    perror(err);
    exit(1);
  }
  struct writer_per_word_apply_arg arg = {f};
  map_apply_arg(file_aliases, file_alias_index_writer, &arg);
  fclose(f);
  printf("> Writing index %s done!n", fname);
}

// output the index to files in the target directory, using the given number of
// shards.
void dump_ii(char *dir, unsigned int shards, int max_parallelism) {
  assert(ii);
  // Write index file, then write shards of output
  write_index(dir, shards);
  // First, we get a list of sorted keys.
  // Then, we sample from the list to divide the output into n shards, selecting
  // key boundaries. Hot keys will skew the shard sizes.
  // Finally, we write the n output shards.
  if (keys != NULL) {
    // no-op, we've done this.
    return;
  }

  keys = NULL;
  n_keys = 0;
  keys_len = 0;

  // populate key list and sort it.
  map_apply(ii, key_aggregate_fn);
  qsort(keys, n_keys, sizeof(char *), cmpstr);

  // TODO: calculate key weights during apply and shard based on weights of keys
  //       in order to generate more even splits.
  // cap shards at number of keys
  shards = min(shards, n_keys);
  // round up to cover the remainder.
  unsigned int stride = n_keys / shards;
  unsigned int rem = n_keys % shards;
  // create an use a threadpool to write files in parallel.
  threadpool_config_t cfg = {max(shards / 2, max_parallelism)};
  threadpool_t tp = threadpool_create(cfg);
  threadpool_start(tp);

  // create writer threads
  struct writer_thread_arg *args =
      malloc(shards * sizeof(struct writer_thread_arg));
  threadpool_work_t work;
  work.fn = writer_thread;
  work.cb = NULL; // no need for a callback
  for (int i = 0; i < shards; i++) {
    // make shards pick up the remainder
    // TODO: this is a mess and is more easily directly calculable. Threads can
    //       easily calculate this themselves, as well.
    int start, wstride;
    if (i < rem) {
      wstride = stride + 1;
      start = i * wstride;
    } else {
      wstride = stride;
      start = rem * (wstride + 1) + (i - rem) * wstride;
    }
    args[i].dir = dir;
    args[i].start = start;
    args[i].stride = wstride;
    args[i].max = n_keys;
    args[i].id = i;
    args[i].shards = shards;
    work.work = &args[i];
    threadpool_add(tp, work);
  }

  // wait for threadpool to drain.
  threadpool_stop(tp, THREADPOOL_STOP_DRAIN);
  threadpool_destroy(tp);
}
