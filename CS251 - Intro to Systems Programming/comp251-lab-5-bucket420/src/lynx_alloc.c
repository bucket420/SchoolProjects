#include "lynx_alloc.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "util.h"

// --------------- Globals ---------------

int malloc_init; // whether the allocator has been initialized. The first call
                 // to malloc will result in lynx_alloc_init() being called,
                 // which will set this value.

region_t *root = NULL; // root region, head of the linked list of regions. The
                       // root is always the most-recently-created region.
                       // Regions are created with calls to region_create().

// Configuration parameters; initialized in lynx_alloc_init().
struct malloc_config config;

// Counters used for debugging; zeroed in lynx_alloc_init().
struct malloc_counters counters;

// mask to convert a block to a region -- converts an address to the least
// multiple of region size less than it.
#define REGION_MASK (~(config.region_size - 1))

// --------------- Helper functions ---------------
// See documentation in helper functions for descriptions of their
// specifications.

// Arithmetic functions.
int is_overflow(size_t a, size_t b, size_t product);
size_t next16(size_t size);
void *align(void *addr);
char atoc16(const char *str);

// Conversion between pointers.
// These functions abstract away the representation of regions and blocks and
// can be used to convert from a block to its enclosing region, a block to its
// size, and a block to/from a data pointer.
region_t *to_region(void *addr);
block_t *to_block(void *data_addr);
size_t block_size(block_t *blk);
void *block_data(block_t *blk);

// Block traversal.
// Used for moving backward/forward between blocks.
block_t *block_next(block_t *blk);
block_t *block_ftr(block_t *blk);
block_t *prev_ftr(block_t *blk);
block_t *prev_block(block_t *blk);

// Block metadata manipulation.
void mark_block_free(block_t *blk);
void mark_block_used(block_t *blk);
int is_used(block_t *blk);
int is_free(block_t *blk);
int is_large(block_t *blk);
void mark_large(block_t *blk);

// Region manipulation.
// Create regions, clean up unused regions, etc.
region_t *region_create();
void clean_regions(block_t *last_blk);
block_t *create_large_block(size_t size);
void free_large_block(block_t *blk);

// Free list manipulation.
// Find free blocks as well as split and merge blocks.
block_t *next_free(size_t desired);
void split(block_t *blk, size_t size);
void merge(block_t *blk);
block_t *merge_left(block_t *blk);
block_t *merge_right(block_t *blk);

// Initialize the allocator.
void lynx_alloc_init();

// Debug functions.
void print_block(block_t *block);
void print_block_list(block_t *block);
void print_region_info(region_t *region, int print_blocks);
void scribble_block(block_t *blk);

// macro for computing a typeless min
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// macro to read config variables from the environment + convert them using a
// given conversion function (strtol, atoi, etc.).
#define GET_CONFIG_VAR(var, conf_name, conv_func)                              \
  do {                                                                         \
    char *tmp = getenv(conf_name);                                             \
    if (tmp) {                                                                 \
      var = conv_func(tmp);                                                    \
    }                                                                          \
  } while (0);

// --------------- Helper function impls ---------------
int is_overflow(size_t a, size_t b, size_t product) {
  // for use in overflow detection
  return a != 0 && product / a != b;
}

region_t *to_region(void *addr) {
  // given a block, mask the low order bits to skip to the region
  uintptr_t ptr = (uintptr_t)addr;
  ptr &= REGION_MASK;
  return (region_t *)ptr;
}

size_t block_size(block_t *blk) {
  // clear low four bits to get block size.
  return *blk & ~0xf;
}

void *block_data(block_t *blk) {
  // raw data starts after block header.
  return (void *)blk + sizeof(block_t);
}

block_t *block_next(block_t *blk) {
  // Starting from a block header, the next block header is at address
  // header address + size.
  //
  // If this is the end of the free list, block size is zero and blk is
  // returned.
  return (block_t *)((void *)blk + block_size(blk));
}

block_t *block_ftr(block_t *blk) {
  // block footer starts before next block's header.
  return ((void *)block_next(blk)) - sizeof(block_t);
}

block_t *prev_ftr(block_t *blk) {
  // return footer block of previous block.
  return (block_t *)((void *)blk - sizeof(block_t));
}

block_t *prev_block(block_t *blk) {
  // return previous block.
  return (block_t *)((void *)blk - block_size(prev_ftr(blk)));
}

block_t *to_block(void *data_addr) {
  // block header precedes address
  return (block_t *)(data_addr - sizeof(block_t));
}

void mark_block_free(block_t *blk) {
  // clear low four bits; here we ignore large blocks because we never mark
  // them used or free.
  *blk &= ~0xf;
  *block_ftr(blk) &= ~0xf;
}

void mark_block_used(block_t *blk) {
  // set low bit
  *blk |= 0x1;
  *block_ftr(blk) |= 0x1;
}

int is_used(block_t *blk) {
  // block is used if 0 bit is set.
  return *blk & 0x1;
}

int is_free(block_t *blk) { return !is_used(blk); }

int is_large(block_t *blk) {
  // bit 1 represents a large block
  return *blk & 0x2;
}

void mark_large(block_t *blk) {
  // set bit 1
  *blk |= 0x2;
}

size_t next16(size_t size) {
  // return the next multiple of 16 > size
  // include enough space for the footer + header of next block
  // 16 + size + (16 - size % 16);
  return 16 + (size | 15) + 1;
}

void *align(void *addr) {
  // return 16-byte aligned address >= addr
  return (void *)(((uintptr_t)addr | 15) + 1);
}

char atoc16(const char *str) { return (char)strtol(str, NULL, 16); }

void lynx_alloc_init() {
  // get config variables
  // region size
  config.region_size = DEFAULT_REGION_SIZE;
  GET_CONFIG_VAR(config.region_size, REGION_SIZE_ENV_VAR, atoi);
  assert(config.region_size % 4096 == 0);
  // block size
  config.max_block_size = MAX_BLOCK_ALLOC;
  GET_CONFIG_VAR(config.max_block_size, MAX_BLOCK_ALLOC_ENV_VAR, atoi);
  // reserve capcity
  config.reserve_capacity = RESERVE_CAPACITY;
  GET_CONFIG_VAR(config.reserve_capacity, RESERVE_CAPACITY_ENV_VAR, atoi);
  assert(config.reserve_capacity % 16 == 0);
  // min split size
  config.min_split_size = MIN_SPLIT_SIZE;
  GET_CONFIG_VAR(config.min_split_size, MIN_SPLIT_ENV_VAR, atoi);
  // scribble char
  config.scribble_char = DEFAULT_SCRIBBLE_CHAR;
  GET_CONFIG_VAR(config.scribble_char, SCRIBBLE_ENV_VAR, atoc16);

  // zero counters
  memset(&counters, 0, sizeof(struct malloc_counters));

  malloc_init = 1;
}

block_t *create_large_block(size_t size) {
  // create a mapped block for the user with the given size
  // large blocks have a 16 byte header; the last 4 bytes of which are used for
  // the block size + metadata. the first 12 bytes are unused.
  // 0      8     16          size
  // | xxxx | size | data ... |
  //        ^      ^
  //        |       ` start of data block
  //         ` metadata is at byte 8
  //
  // note that unlike blocks allocated within a region, the size of a large
  // block represents its TOTAL size, including padding, rather than the size of
  // the data block. this is because we use this size when we unmap the region.

  // reserve space for block metadata.
  size_t adjusted_size = next16(size);
  assert((uint32_t)adjusted_size == adjusted_size);
  // map the block
  void *addr = mmap(NULL, adjusted_size, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (addr == MAP_FAILED) {
    return NULL;
  }
  // data starts at byte 4 of a large block
  void *data_start = addr + 16;
  block_t *blk = to_block(data_start);
  // set metadata
  *blk = adjusted_size;
  mark_large(blk);

  if (config.scribble_char)
    scribble_block(blk);

  return blk;
}

void free_large_block(block_t *blk) {
  // simply unmap the large block; metadata contains the size for the region.
  void *addr = block_data(blk) - 16;
  munmap(addr, block_size(blk));
}

void scribble_block(block_t *blk) {
  size_t size = block_size(blk);
  char *data = block_data(blk);
  size_t scribble_distance =
      is_large(blk) ? size - 16 : size - 2 * sizeof(block_t);
  memset(data, config.scribble_char, scribble_distance);
}

region_t *region_create() {
  // mmap a new region
  void *addr = mmap(NULL, config.region_size, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (addr == MAP_FAILED) {
    return NULL;
  }
  if ((uintptr_t)addr % config.region_size != 0) {
    // we have the requirement that the address that a region starts at is a
    // multiple of the region size. for single-page regions this is always the
    // case. however, we may have a region size that spans multiple pages. in
    // this case, we may need to reallocte the region in order to align it with
    // a multiple of the region size.
    //
    // here we handle the case where memory is not aligned; our approach to
    // fixing this is to request a bigger chunk and then extract an aligned
    // region from the bigger chunk of memory.
    munmap(addr, config.region_size); // give back our misaligned allocation
    // ask for enough to guarantee that we have an aligned segment of the memory
    // we receive (2x region_size).
    addr = mmap(NULL, 2 * config.region_size, PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (addr == MAP_FAILED) {
      // this is a hard requirement for this allocator, we must have an aligned
      // region. if we cannot get 2x a region, we do not attempt again.
      return NULL;
    }
    if ((uintptr_t)addr % config.region_size == 0) {
      // if we got back an aligned address, just give back the second half of
      // the region.
      munmap(addr + config.region_size, config.region_size);
    } else {
      // the start address is not aligned, but we must have some region sized
      // segment that is aligned. here we find the nearest multiple of
      // region_size and free the (1) preceding + (2) succeeding pages.
      //
      // (1) give back preceding unaligned region
      void *start = addr;
      addr =
          addr + (config.region_size - ((uintptr_t)addr % config.region_size));
      size_t change = addr - start;
      munmap(start, addr - start);
      // (2) give back succeeding unaligned region; we are guaranteed that this
      // is nonzero because we asked for double the region size and addr was not
      // a multiple of the region size.
      void *end = addr + config.region_size;
      change = config.region_size - change;
      munmap(end, change);
      // well that was fun.
    }
  }

  // now, initialize the region
  region_t *tmp = (region_t *)addr;
  tmp->n_free = 1;
  tmp->n_used = 0;
  tmp->next = NULL;
  tmp->prev = NULL;
  // we initialize block_list after computing the data start

  // create initial+final blocks and the first free block
  //
  // initial block starts at next 16-byte aligned address
  // | initial         | free                   |
  // | hdr | ftr | hdr | free block | ftr | hdr |
  //       ^           ^                     `- size 0 -- final block
  //        \__________|_
  //                     `- 16-byte aligned
  void *blk_data = align(addr + sizeof(region_t) + sizeof(block_t));
  void *next_data = align(blk_data + 1);
  uintptr_t blk_size = next_data - blk_data;

  // write first block and mark it used.
  block_t *blk = to_block(blk_data);
  *blk = blk_size;
  *block_ftr(blk) = blk_size;
  mark_block_used(blk);

  // finish initialization of region.
  tmp->block_list = blk;

  // write next block and mark it free.
  blk = to_block(next_data);
  // this block's size is the size from the greatest multiple of 16 less than
  // the end of the region.
  blk_size = (addr + config.region_size) - next_data;

  *blk = blk_size;
  *block_ftr(blk) = blk_size;
  mark_block_free(blk); // should be a no-op, but let's be explicit.

  // write last block and mark it used.
  blk = block_next(blk); // header of next block
  *blk = 1;              // block is size 0 and used

  counters.region_allocs += 1;

  return tmp;
}

void clean_regions(block_t *last_blk) {
  // Clean up any used regions, using the last freed block as a hint.
  // For this implementation: If regions are reaped as soon as they the last
  // block is freed, the only block that requires cleanup is the one
  // containing the last freed block.
  if (to_region(last_blk)->n_used) {
    return;
  }
  // block is empty, unlink region and delete
  region_t *del = to_region(last_blk);
  if (del->prev) {
    assert(del->prev->next == del);
    del->prev->next = del->next;
    // del cannot be root
    assert(del != root);
  }
  if (del->next) {
    assert(del->next->prev == del);
    del->next->prev = del->prev;
    // del could be root
  }
  root = del == root ? del->next : root;
  assert(root != del);
  munmap(del, config.region_size);
  counters.region_frees += 1;
}

block_t *next_free(size_t desired) {
  // find the next free block
  region_t *cur = root;
  block_t *ret = NULL;
  while (1) {
    // find the next region with at least one free block
    while (cur && cur->n_free < 1) {
      cur = cur->next;
    }
    if (!cur) {
      // no more free regions; stop searching.
      ret = NULL;
      break;
    }
    // find free block in region
    block_t *blk = cur->block_list;
    while (is_used(blk) || block_size(blk) < desired) {
      block_t *new_blk = block_next(blk);
      if (new_blk == blk) {
        // we reached the end of the region; there are no free blocks that fit
        // the alloc request. break out and try the next region.
        break;
      }
      blk = new_blk;
    }
    if (block_size(blk) >= desired) {
      // block found, we're done.
      ret = blk;
      break;
    }
    // try next region
    cur = cur->next;
  }
  return ret;
}

block_t *merge_left(block_t *blk) {
  // attempt to merge this block with the block to its left. if successful,
  // return the newly merged block.
  // our boundary condition is the intro block. the intro block is a real
  // block of size 16 and is always used.

  // the block to the left
  block_t *left_block = prev_block(blk);
  if (is_used(left_block)) {
    // if the left block is used, return
    return blk;
  } else {
    // merging this block and left block by updating metadata of the left block
    size_t new_size = block_size(blk) + block_size(left_block);
    *left_block = new_size;
    *block_ftr(left_block) = new_size;
    // update region metadata
    to_region(blk)->n_free -= 1;
    // attempt to merge the next left block
    return merge_left(left_block);
  }
}

block_t *merge_right(block_t *blk) {
  // attempt to merge this block with the block to its right. return the new
  // merged block (which should always be this block).

  // the block to the right
  block_t *right_block = block_next(blk);
  if (is_used(right_block)) {
    // if the right block is used, return
    return blk;
  } else {
    // merging this block and right block by updating metadata of this block
    size_t new_size = block_size(blk) + block_size(right_block);
    *blk = new_size;
    *block_ftr(blk) = new_size;
    // update region metadata
    to_region(blk)->n_free -= 1;
    // attempt to merge the next right block
    return merge_right(blk);
  }
}

void merge(block_t *blk) {
  // recursively merge blocks.
  // 1. Check previous -- merge.
  blk = merge_left(blk);
  // 2. Check following -- merge.
  blk = merge_right(blk);
}

void split(block_t *blk, size_t size) {
  // Given a free block and a desired size, determine whether to split the
  // block.
  // make sure that size is a multiple of 16
  if (size % 16)
    size = next16(size);
  // size of the second block
  size_t remaining_size = block_size(blk) - size;
  if (remaining_size < config.min_split_size || (int)remaining_size < 0) {
    // do nothing if the second block is too small
    return;
  }
  // update the first block's metadata
  *blk = size;
  *block_ftr(blk) = size;
  // update the second block's metadata
  *block_next(blk) = remaining_size;
  *block_ftr(block_next(blk)) = remaining_size;
  // update region metadata
  to_region(blk)->n_free += 1;
}

// --------------- Malloc impl functions ---------------

// MALLOC
void *lynx_malloc(size_t size) {
  if (!malloc_init) {
    // perform any one-time initialization
    lynx_alloc_init();
  }
  // return NULL if size is 0
  if (!size) {
    return NULL;
  }
  // create a large block if size (including header and footer) exceeds the
  // threshold
  if (size + 8 > config.max_block_size) {
    // update accounting
    counters.large_block_allocs += 1;

    return block_data(create_large_block(size % 16 ? next16(size) : size));
  }
  // allocate a normal block when size is within range
  // reserve space for header and footer,
  size += 8;
  // attempt to allocate the next free block
  block_t *blk = next_free(size);
  if (blk) {
    // if there's a free block, attempt to split it
    split(blk, size);
    // mark used
    mark_block_used(blk);
    // update region metadata
    to_region(blk)->n_free -= 1;
    to_region(blk)->n_used += 1;
  } else {
    // no free block found, attempt to create a new region
    region_t *new_region = region_create();
    if (!new_region) {
      // no region can be created, allocation failed
      return NULL;
    }
    // a new region is successfully created
    if (root) {
      // if there are existing regions, prepend the region to the list
      root->prev = new_region;
      root->prev->next = root;
      root = root->prev;
    } else {
      // if there's no existing region, the new region is the root
      root = new_region;
    }
    // a free block is guaranteed now
    blk = next_free(size);
    // split the block
    split(blk, size);
    // mark used
    mark_block_used(blk);
    // update region metadata
    root->n_free -= 1;
    root->n_used += 1;
  }
  // scribble if neccessary
  if (config.scribble_char)
    scribble_block(blk);

  // update accounting
  counters.total_allocs += 1;

  return block_data(blk);
}

// FREE
void lynx_free(void *ptr) {
  if (!ptr) {
    // "If ptr is NULL, no operation is performed."
    return;
  }

  block_t *blk = to_block(ptr);

  if (is_large(blk)) {
    // follow the large block path.
    counters.large_block_frees += 1;
    free_large_block(blk);
    return;
  }

  assert(is_used(blk));

  // free the block before trying to merge it with neighbors.
  mark_block_free(blk);

  // update accounting for this block
  to_region(blk)->n_free += 1;
  to_region(blk)->n_used -= 1;
  counters.total_frees += 1;

  // try to merge free space
  merge(blk);

  // try to clean up blocks
  clean_regions(blk);
}

// CALLOC
void *lynx_calloc(size_t nmemb, size_t size) {
  if (!nmemb || !size) {
    // "If nmemb or size is 0, then calloc() returns either NULL or a unique
    // pointer that can later be successfully passed to free"
    // nb: our implementation of malloc above will return NULL, but let's just
    // be explicit.
    return NULL;
  }
  void *addr = lynx_malloc(nmemb * size);
  memset(addr, 0, nmemb * size);
  return addr;
}

// REALLOC
void *lynx_realloc(void *ptr, size_t size) {
  if (!ptr) {
    // "If ptr is NULL the call is equivalent to malloc(size), for all values
    // of size" (including zero)
    return lynx_malloc(size);
  }
  if (size == 0) {
    // "If size is equal to zero and ptr is not NULL, then the call is
    // equivalent to free(ptr)"
    lynx_free(ptr);
    return NULL;
  }

  block_t *blk = to_block(ptr);
  // TODO: optionally shrink allocation; currently only shrink when moving
  // from a large block to a small one.
  if (block_size(blk) - 16 > size &&
      !(is_large(blk) && size + 32 < config.max_block_size)) {
    assert(ptr);
    return ptr;
  }
  void *new_ptr = lynx_malloc(size);
  if (!new_ptr) {
    return NULL;
  }
  size_t cp_size = MIN(block_size(blk) - 16, size);
  memcpy(new_ptr, ptr, cp_size);
  lynx_free(ptr);
  return new_ptr;
}

// REALLOCARRAY
void *lynx_reallocarray(void *ptr, size_t nmemb, size_t size) {
  if (is_overflow(nmemb, size, nmemb * size)) {
    // "reallocarray() fails safely in the case where the multiplication would
    // overflow. If such an overflow occurs, reallocarray() returns NULL, sets
    // errno to ENOMEM, and leaves the original block of memory unchanged."
    errno = ENOMEM;
    return NULL;
  }
  return lynx_realloc(ptr, nmemb * size);
}

// Debug configs and functions.

struct malloc_counters lynx_alloc_counters() {
  return counters;
}
struct malloc_config lynx_alloc_config() {
  return config;
}

// Warning: calling these print functions from a program that uses this as its
// allocator implementation will result in calls to this allocator (printf
// uses malloc).
//
// This is fine unless you are debugging this allocator, in which case
// counters and assertions should be used for instrumentation.
void print_block(block_t *block) {
  // print a block's metadata
  printf("\t\t [%p - %p] (size %4zu) status: %s\n", block_data(block),
         block_data(block) + block_size(block), block_size(block),
         is_free(block) ? "free" : "used");
}

void print_block_list(block_t *block) {
  // print all blocks starting from the given block
  while (block && block_size(block)) {
    print_block(block);
    block = (void *)block + block_size(block);
  }
}

void print_region_info(region_t *region, int print_blocks) {
  // print metadata about a region and then print its blocks
  printf("Region %p:\n", region);
  printf("\tnext: %p\n", region->next);
  printf("\tn_free: %d\n", region->n_free);
  printf("\tblock_list:\n");
  if (print_blocks) {
    print_block_list(region->block_list);
  }
}

#define DUMP_VAR(counter) printf("%-20s : %lu\n", "" #counter "", counter);

void print_lynx_alloc_debug_info() {
  // print all allocator debug information
  printf("----🐯 lynx allocator debug info start 🐯----\n");
  if (malloc_init) {
    printf("Config:\n");
    DUMP_VAR(config.region_size);
    DUMP_VAR(config.max_block_size);
    printf("%-20s : %02hhx\n", "config.scribble_char", config.scribble_char);
    printf("Regions:\n");
    region_t *tmp = root;
    while (tmp) {
      print_region_info(tmp, /*print_blocks=*/1);
      tmp = tmp->next;
    }
    printf("Counters:\n");
    DUMP_VAR(counters.region_allocs);
    DUMP_VAR(counters.region_frees);
    DUMP_VAR(counters.total_allocs);
    DUMP_VAR(counters.total_frees);
    DUMP_VAR(counters.large_block_allocs);
    DUMP_VAR(counters.large_block_frees);
  } else {
    printf("Uninitialized.\n");
  }
  printf("----🐯 lynx allocator debug info end 🐯----\n");
}
