#ifndef __MAP_H__
#define __MAP_H__

#include <stdint.h>

// Implementation of a hash map with chaining.
// Usage:
//
//  map_t *map = map_create(init_size);
//
//  is_new = map_put(string_key, value_ptr);
//
//  was_in = map_remove(map, string_key);
//
//  val_ptr = NULL;
//  is_in = map_get(map, &val_ptr);
//  // *val_ptr is value if is_in is nonzero.
//
//  map_metrics_t metrics = map_metrics(map);
//  map_resize(map, new_size);
//
//  map_free(&map);

// Forward declaration of Map type.
typedef struct map_t map_t;

// Metrics type.
// Used to collect statistics about map entries. Returned by map_metrics().
typedef struct map_metrics_t {
  uint32_t max_depth;
  uint32_t num_entries;
  uint32_t curr_size;
} map_metrics_t;

// Creates a map with the given initial size.
//
// Caller owns returned pointer; must be freed with free_map.
// Must be freed with map_free.
map_t *map_create(uint32_t init_size);

// Get metrics about the current map state.
//
// Caller owns returned pointer.
map_metrics_t *map_metrics(map_t *map);

// Frees a map.
//
// Frees any entry metadata (including keys) and m. m will be NULL upon return.
void map_free(map_t **map);

// Resizes the map to the given size.
void map_resize(map_t *map, uint32_t new_size);

// Puts entry into map.
//
// Returns nonzero if the key was new. If key was already present in map,
// returns 0 and updates entry to new_value.
//
// Makes a copy of the given key to store internally. This key is used in
// map_apply(). Internal key is freed when map is destroyed.
int map_put(map_t *map, const char *key, void *new_value);

// Removes entry from map.
//
// Returns nonzero if key was removed. Returns 0 if key was not present. Frees
// copy of key. Does NOT free value.
int map_remove(map_t *map, const char *key);

// Gets a value from the map.
//
// Sets value_ptr to stored void* value or NULL, if the entry is not present.
// Returns nonzero if key was present and 0 otherwise.
//
// If value_ptr is NULL, only performs a presence test.
int map_get(map_t *map, const char *key, void **value_ptr);

// Apply a function to each value in the map, modifying its value.
//
// Applies apply_fn to each value in the map and replaces the map value with the
// returned value. If apply_fn replaces the value, apply_fn must take ownership
// of previous value.
void map_apply(map_t *map, void *apply_fn(const char *key, void *value));

// Debug function.
//
// This is a free function for you to define however you want in order to debug
// your map with the interactive tester.
void map_debug(map_t *map);

#endif
