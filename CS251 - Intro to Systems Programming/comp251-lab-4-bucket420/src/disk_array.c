#include "disk_array.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

// Macro that evaluates to the size of the header for disk-backed arrays.
#define HDR_SIZE (sizeof(uint64_t) + sizeof(uint64_t))

// Given the base address of the file, return a pointer to the location in the
// header that represents the number of elements.
static inline uint64_t *n_el(void *base_address) {
  return (uint64_t *)base_address;
}

// Given the base address of the file, return a pointer to the location in the
// header that represents the size of each element.
static inline uint64_t *el_size(void *base_address) {
  return (uint64_t *)(base_address + sizeof(uint64_t));
}

void array_open(const char *fname, uint64_t desired_elements,
                uint64_t element_size, disk_array_t *arr) {
  assert(arr);
  char *tpath = malloc(strlen(fname) + 5);
  strcpy(tpath, fname);
  strcat(tpath, ".arr");
  DEBUG_PRINT("opening %s\n", tpath);

  size_t desired_size = 0;

  // open the mmap'd region.
  if (desired_elements) {
    // element size must be given.
    assert(element_size);
    // get a block at the beginning for the number and size of array elements.
    desired_size = desired_elements * element_size + HDR_SIZE;
  }
  mm_open(tpath, desired_size, &arr->mm_region);
  free(tpath);

  void *base = arr->mm_region.start;
  size_t size =
      arr->mm_region.size; // the authoritative size comes from the region.

  // initialize arr fields.
  // start of array is after header fields
  arr->array = base + HDR_SIZE;

  if (desired_size) {
    // if the array is new, zero upon opening.
    memset((char *)base, 0, size);
    // set the number of elements (in the header) to be the size of the file
    // (minus the header) / the size of each element. If not evenly divisible,
    // truncates the bytes in the suffix.
    *n_el(base) = (size - HDR_SIZE) / element_size;
    // set the size of each element (in the header) to be the given element
    // size.
    *el_size(base) = element_size;
  }

  // Read the number of elements and the element size from the header.
  arr->n = n_el(base);
  arr->element_size = el_size(base);
}

void array_close(disk_array_t *arr) {
  // Nothing to do but close the memory region.
  mm_close(&arr->mm_region);
}
