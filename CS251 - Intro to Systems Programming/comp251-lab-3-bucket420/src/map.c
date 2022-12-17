#include "map.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fnv64.h"

// Forward declarations. Types follow.
typedef struct entry_t entry_t;
typedef struct map_t map_t;

// Uncomment #define DEBUG and comment out the following to turn on DEBUG_PRINT
// statements. See the macro definition below.
#undef DEBUG
// #define DEBUG

// Definition of DEBUG_PRINT macro. Whenever you want to insert debug print
// statements, you should use this macro. It behaves _exactly_ like printf. If
// you want to print the variables key and hashed_key, for example,
// you could do the following:
//
//    DEBUG_PRINT("key: %s; hash: %016lx\n", key, hashed_key);
//
// This will behave like printf when the #define DEBUG statement above is
// uncommented. When the #undef DEBUG statement is uncommented, nothing will be
// printed. You should use DEBUG when you are trying to drill down into a
// problem, and then turn off DEBUG when you are running your final experiments
// (since these perform millions of operations on your map).
#ifdef DEBUG
#define DEBUG_PRINT(format, args...)                                           \
  fprintf(stderr, "D %s:%d:%s(): " format, __FILE__, __LINE__, __func__, ##args)
#else
#define DEBUG_PRINT(fmt, args...) // no-op
#endif

// Entry struct.
typedef struct entry_t {
  struct entry_t *next; // next entry in linked list
  struct entry_t *prev; // previous entry
  char *key;            // string key
  uint64_t hkey;        // 64-bit hash of key
  void *value;          // value stored for the key
} entry_t;

// Map struct.
struct map_t {
  entry_t **entries; // array of pointers to entries
  uint32_t n;        // size of entries array
};

// Helper functions
entry_t *find_key_in_chain(entry_t *curr, uint64_t hkey) {
  while (curr) {
    if (curr->hkey == hkey)
      return curr;
    curr = curr->next;
  }
  return NULL;
}

void entries_free(entry_t **entries, uint32_t size) {
  for (int i = 0; i < size; i++) {
    entry_t *curr = entries[i];
    while (curr) {
      free(curr->key);
      entry_t *temp = curr->next;
      free(curr);
      curr = temp;
    }
  }
  free(entries);
}

void set_entry(entry_t *entry, entry_t *next, entry_t *prev, const char *key,
               uint64_t hkey, void *new_value) {
  entry->next = next;
  entry->prev = prev;
  entry->key = malloc((strlen(key) + 1) * sizeof(char));
  strncpy(entry->key, key, strlen(key) + 1);
  entry->hkey = hkey;
  entry->value = new_value;
}

map_t *map_create(uint32_t init_size) {
  map_t *map = malloc(sizeof(map_t));
  map->entries = calloc(init_size, sizeof(entry_t *));
  map->n = init_size;
  return map;
}

map_metrics_t *map_metrics(map_t *map) {
  map_metrics_t *stats = malloc(sizeof(map_metrics_t));
  stats->num_entries = 0;
  stats->max_depth = 0;
  stats->curr_size = map->n;
  int depth = 0;
  for (int i = 0; i < map->n; i++) {
    entry_t *curr = map->entries[i];
    while (curr) {
      depth += 1;
      curr = curr->next;
    }
    if (depth > stats->max_depth)
      stats->max_depth = depth;
    stats->num_entries += depth;
    depth = 0;
  }
  return stats;
}

void map_free(map_t **map) {
  entries_free((*map)->entries, (*map)->n);
  free(*map);
  *map = NULL;
}

int map_put(map_t *map, const char *key, void *new_value) {
  uint64_t hkey = fnv64(key);
  uint64_t index = hkey % map->n;
  if (!map->entries[index]) {
    map->entries[index] = malloc(sizeof(entry_t));
    set_entry(map->entries[index], NULL, NULL, key, hkey, new_value);
    return 1;
  }
  entry_t *found = find_key_in_chain(map->entries[index], hkey);
  if (found) {
    found->value = new_value;
    return 0;
  } else {
    entry_t *new_entry = malloc(sizeof(entry_t));
    set_entry(new_entry, map->entries[index], NULL, key, hkey, new_value);
    (map->entries[index])->prev = new_entry;
    map->entries[index] = new_entry;
    return 1;
  }
}

int map_remove(map_t *map, const char *key) {
  uint64_t hkey = fnv64(key);
  uint64_t index = hkey % map->n;
  entry_t *found = find_key_in_chain(map->entries[index], hkey);
  if (found) {
    if (found->next)
      found->next->prev = found->prev;
    if (found->prev)
      found->prev->next = found->next;
    else
      map->entries[index] = found->next;
    free(found->key);
    free(found);
    return 1;
  }
  return 0;
}

int map_get(map_t *map, const char *key, void **value_ptr) {
  uint64_t hkey = fnv64(key);
  uint64_t index = hkey % map->n;
  entry_t *found = find_key_in_chain(map->entries[index], hkey);
  if (found) {
    if (value_ptr)
      *value_ptr = found->value;
    return 1;
  }
  *value_ptr = NULL;
  return 0;
}

void map_resize(map_t *map, uint32_t new_size) {
  entry_t **old_entries = map->entries;
  uint32_t old_n = map->n;
  map->entries = calloc(new_size, sizeof(entry_t));
  map->n = new_size;
  for (int i = 0; i < old_n; i++) {
    entry_t *curr = old_entries[i];
    while (curr) {
      map_put(map, curr->key, curr->value);
      curr = curr->next;
    }
  }
  entries_free(old_entries, old_n);
}

void map_debug(map_t *map) {
  for (int i = 0; i < map->n; i++) {
    entry_t *curr = map->entries[i];
    printf("%d ", i);
    while (curr) {
      printf("(%s, %s) -> ", curr->key, (char *)curr->value);
      curr = curr->next;
    }
    printf("X\n");
  }
}

void map_apply(map_t *map, void *apply_fn(const char *key, void *value)) {
  // For each table entry.
  for (int i = 0; i < map->n; i++) {
    entry_t *cur = map->entries[i];
    // Traverse the entries at i, calling apply_fn.
    while (cur) {
      cur->value = apply_fn(cur->key, cur->value);
      cur = cur->next;
    }
  }
}
