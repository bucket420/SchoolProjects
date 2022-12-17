#ifndef __DARRAY_H__
#define __DARRAY_H__

#include <stdint.h>

#include "mm_util.h"

typedef struct disk_array_t disk_array_t;

// ----------------------------------------------
// disk array usage and file format/memory layout
// ----------------------------------------------
//
// Disk arrays are arrays that can be accessed like any other array, but are
// disk-backed. Any updates to the array are written through to disk.
//
// Disk arrays can be thought of as a thin layer on top of a memory-mapped file,
// but have some built-in error checking and abstract away the mmap interface.
//
// Opening/closing an array is done with array_open/array_close. The disk array
// struct contains a pointer to the array, and metadata about its contents and
// limits.
//
// Typical usage:
//
//    uint64_t type_size_bytes = sizeof(my_array_type);
//    my_array_type *array = NULL;
//
//    disk_array_t disk_array;
//    array_open(filename, desired_num_elements, type_size_bytes, &disk_array);
//
//    // Get array pointer.
//    array = (my_array_type*) disk_array.array;
//    ...
//    // Use array like normal.
//    array[i] = ...
//    ...
//    array_close(&disk_array);
//
// The memory layout and file format of a disk-backed array starts with a header
// representing the number of elements (as a 64-bit unsigned integer) and the
// size of each element (also a 64-bit unsigned integer). A data section follows
// the header.
//
// | uint64_t num elements | uint64_t element size | data .... |
//
// The size of the data section is the product of the number of elements and the
// element size.

struct disk_array_t {
  void *array;            // pointer to start of array
  uint64_t *n;            // pointer to length of array
  uint64_t *element_size; // pointer to size in bytes of each element
  mm_region_t mm_region;  // memory mapped region data
};

// Open a disk-backed array.
// desired_elements should be 0 if opening an existing file. If desired_elements
// is nonzero, a new file will be created.
void array_open(const char *fname, uint64_t desired_elements,
                uint64_t element_size, disk_array_t *arr);

// Close a disk-backed array.
void array_close(disk_array_t *arr);

#endif
