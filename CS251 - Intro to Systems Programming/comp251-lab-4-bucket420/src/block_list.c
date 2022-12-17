#include "block_list.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mm_util.h"
#include "util.h"

// Helper macro that takes an address/pointer argument and dereferences it as a
// pointer to an unsigned 32-bit integer.
#define AS_INT(expr) *((uint32_t *)(expr))
// Helper macro that takes an address/pointer argument and an offest and
// dereferences (address+offset) as an unsigned 32-bit integer.
#define AS_INT_OFFSET(expr, offset) *((uint32_t *)((expr) + (offset)))

void bl_open(const char *fname, uint32_t size, block_list_t *lst) {
  assert(lst);
  assert(size ? size > 4 * sizeof(uint32_t) : 1);
  char *tpath = malloc(strlen(fname) + 5);
  strcpy(tpath, fname);
  strcat(tpath, ".ll");
  DEBUG_PRINT("opening %s\n", tpath);

  // mmap the file.
  mm_open(tpath, size, &lst->mm_region);
  if (size) {
    // if this is a new file, clear contents
    memset(lst->mm_region.start, 0, size);
  }
  lst->tail = NULL; // tail is uninitialized.
  free(tpath);
}

void bl_close(block_list_t *lst) {
  // Just close the mmap-ed region.
  mm_close(&lst->mm_region);
}

// Helper function that, given the start address of a block, returns the start
// address of the next block.
void *next(void *start) { return start + AS_INT(start) + 2 * sizeof(uint32_t); }

// Helper function to initialize the tail of a block list.
void init_tail(block_list_t *lst) {
  if (lst->tail) {
    // tail is initialized; skip
    return;
  }
  // if tail is uninitialized, find the tail.
  DEBUG_PRINT("finding tail...\n");
  // start at first real block (skip 8 bytes zero padding).
  void *start = lst->mm_region.start + 2 * sizeof(uint32_t);
  // if block header is nonzero, this is not the tail.
  while (AS_INT(start)) {
    // advance to the next block.
    start = next(start);
  }
  // we've reached a header with size zero, this is the tail.
  lst->tail = start;
  DEBUG_PRINT("tail offset: %ld\n", lst->tail - lst->mm_region.start);
}

char *bl_append(char *block, uint32_t block_size, block_list_t *lst) {
  assert(block);
  assert(block_size);

  // initialize tail, as we need to append there.
  init_tail(lst);

  // The tail is 8 bytes -- a zero-size block. To add this new block, we need to
  // be able to add the size of the block, its header/footer, and still have 8
  // bytes for the tail. This is 16 bytes (4*sizeof(uint32_t)) + block_size +
  // current tail. Make sure we do not exceed the range of our memory-mapped
  // region.
  // TODO: we technically do not need a full empty block for the tail -- just
  // one 4-byte null terminator. Here we keep a spare 4 bytes for symmetry with
  // the head and simplicity of the format description.
  if (lst->tail + block_size + 4 * sizeof(uint32_t) >
      lst->mm_region.start + lst->mm_region.size) {
    // new tail would be outsie of possible range!
    return NULL;
  }

  // set tail value to be the new block size.
  AS_INT(lst->tail) = block_size;

  // copy the block to the tail's data region.
  void *data_start = lst->tail + sizeof(uint32_t);
  memcpy(data_start, block, block_size);

  // update the tail to point past the block we just added.
  lst->tail = lst->tail + block_size + 2 * sizeof(uint32_t);

  // create block footer and new tail.
  AS_INT_OFFSET(lst->tail, -(int)sizeof(uint32_t)) = block_size;
  AS_INT(lst->tail) = 0;
  AS_INT_OFFSET(lst->tail, sizeof(uint32_t)) = 0;

  return data_start;
}

char *bl_next(char *last, uint32_t *block_size, block_list_t *lst) {
  if (!last) {
    // last is null, so we are starting a new traversal.
    // start at start of head block (size 0).
    last = lst->mm_region.start + sizeof(uint32_t);
  }
  // skip past the previous block: last + tail pointer + head pointer + size of
  // block.
  // TODO: just use next helper fn here.
  last =
      last + 2 * sizeof(uint32_t) + AS_INT_OFFSET(last, -(int)sizeof(uint32_t));
  // header is now behind us.
  *block_size = AS_INT_OFFSET(last, -(int)sizeof(uint32_t));
  if (!*block_size) {
    // at tail, return NULL
    // TODO: as an optimization, make this initialize lst->tail if it has not
    //       already been initialized.
    return NULL;
  }
  return last;
}

char *bl_prev(char *last, uint32_t *block_size, block_list_t *lst) {
  if (!last) {
    // last is null, so we are starting a new traversal.
    // initialize the tail pointer if we already haven't done so.
    init_tail(lst);
    // start at start of tail
    last = (char *)(lst->tail + sizeof(uint32_t));
  }
  // tail of previous block is 2 ints behind
  *block_size = AS_INT_OFFSET(last, 2 * -(int)sizeof(uint32_t));
  if (!*block_size) {
    // at head, return NULL
    return NULL;
  }
  // Go back a block and two ints.
  last = (char *)(last - 2 * sizeof(uint32_t) - *block_size);
  return last;
}
