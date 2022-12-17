#ifndef __MAP_H__
#define __MAP_H__

#include <stdint.h>

// Implementation of a hash map with chaining.
//
// Usage:
// ------
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
//
// Thread safety:
// --------------
//
//   Maps are threadsafe; there is a global mutex that protects the underlying
//   hashtable's global state (number of entries, etc.) and each row has a
//   per-row mutex for row-wise mutations (get/put/remove/etc.).
//
//   A map supports a get/put operation that allows a user to either retrieve
//   or put a new entry when no such element exists. This allows for a user to
//   query the map and conditionally put a new entry in the map as a single
//   atomic operation.

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

// Retrieves or adds a zero value to a map, atomically.
//
// Sets value_ptr to stored value returns nonzero if key was present. In this
// case, zero is ignored.  If entry is not present, value_ptr will point to
// zero and 0 will be returned.
int map_get_or_put(map_t *map, const char *key, void **value_ptr, void *zero);

// Apply a function to each value in the map, modifying its value.
//
// Applies apply_fn to each value in the map and replaces the map value with the
// returned value. If apply_fn replaces the value, apply_fn must take ownership
// of previous value.
void map_apply(map_t *map, void *apply_fn(const char *key, void *value));

// Apply a function to each value in the map, modifying its value.
//
// This variant accepts an additional parameter that can be passed to the
// apply_fn.
//
// Applies apply_fn to each value in the map and replaces the map value with the
// returned value. If apply_fn replaces the value, apply_fn must take ownership
// of previous value.
void map_apply_arg(map_t *map,
                   void *apply_fn(const char *key, void *value, void *arg),
                   void *arg);

// Debug function.
//
// This is a free function for you to define however you want in order to debug
// your map with the interactive tester.
void map_debug(map_t *map);

#endif
