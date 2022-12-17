#ifndef __BLOCK_LIST_H__
#define __BLOCK_LIST_H__

#include <stdint.h>

#include "mm_util.h"

typedef struct block_list_t block_list_t;

// ----------------------------------------
// block list file format and memory layout
// ----------------------------------------
//
// Block lists are append-only in-memory file-backed linked lists of
// blobs/blocks of bytes. A block list has a maximum size, which is supplied at
// create time, but may store a variable number of blocks, depending on size of
// the blocks.
//
// Typical usage:
//
//    block_list_t list;
//    bl_open(filename, desired_size_bytes, &list);
//    ...
//    // Append a block
//    bl_append(buffer, block_size, &list);
//    ...
//    // Iterate over blocks, starting at head
//    uint32_t cur_size = 0;
//    char *cur_block = bl_next(NULL, &cur_size, &list);
//    while (cur_block) {
//      // process block
//      cur_block = bl_next(cur_block, &cur_size, &list);
//    }
//    ...
//    bl_close(&list);
//
// The memory/file format consists of blocks, each of which has a header and a
// footer marker. Both header and footer contain the size of the block. All
// blocks are at least one byte.
//
// The file starts with a size 0 block and ends with a size 0 block.
//
// For example (block sizes are examples only):
//
// | 0 | 0 | 128 | 128 bytes | 128 | 1928 | 1928 bytes | 1928 | ... | 0 | 0 |
// | dummy | h0  |  block 0  | f0  |  h1  |  block 1   |  f1  | ... |  end  |
//
// Internally, blocks are navigated as a linked list, by examining the
// header/footer of blocks in order to determine where to find the next
// header/footer.

// block list struct.
struct block_list_t {
  mm_region_t mm_region; // memory-mapped region data
  void *tail; // pointer to list tail (do not read, may not be initialized).
};

// Open a disk-backed append-only block list format.
//
// If table exists, size should be zero.  When size is nonzero, the table will
// be created with the given size.
void bl_open(const char *fname, uint32_t size, block_list_t *lst);

// Close a list.
void bl_close(block_list_t *lst);

// Append an element to a block list.
//
// block should be a pointer to the block to append, and block_size is the size
// of the block to append. Returns pointer to block if append succeeded, or NULL
// if the list was full.
char *bl_append(char *block, uint32_t block_size, block_list_t *lst);

// Reads a block from the list.
//
// Blocks are read in order. last should be a pointer returned by
// bl_next/bl_prev, or NULL, if reading from the start (or end) of the list.
// block_size will be set to the size of the returned block.
//
// At the beginning/end of the list, returns NULL and size will be 0.
char *bl_next(char *last, uint32_t *block_size, block_list_t *lst);
char *bl_prev(char *last, uint32_t *block_size, block_list_t *lst);

#endif
