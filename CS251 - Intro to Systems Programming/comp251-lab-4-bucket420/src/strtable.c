#include "strtable.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

void strtable_open(char *path, uint32_t create_size, strtable_t *tbl) {
  assert(tbl);

  // open backing file
  char *tpath = malloc(strlen(path) + 5);
  strcpy(tpath, path);
  strcat(tpath, ".stb");
  DEBUG_PRINT("opening %s\n", tpath);

  mm_open(tpath, create_size, &tbl->mm_region);

  free(tpath);

  // metadata is stored in table; set metadata pointer to point to the start of
  // the region.
  tbl->metadata = tbl->mm_region.start;
  // elements begin right past the metadata; set elements pointer to point to
  // the first byte past the metadata.
  tbl->elements = tbl->mm_region.start + sizeof(struct table_metadata);

  // if table is being created, initialize the header.
  if (create_size) {
    DEBUG_PRINT("initializing header\n");
    // add header characters
    memcpy((char *)tbl->metadata, "STBL", 4);
    // there are initially no elements in the table
    tbl->metadata->len = 0;
    tbl->metadata->size = create_size;
  }

  // validate that this is a strtable.
  assert(strncmp((char *)tbl->metadata, "STBL", 4) == 0);

  // validate that size was stored correctly.
  assert(tbl->metadata->size == tbl->mm_region.size);
}

void strtable_close(strtable_t *tbl) {
  // simply close the memory-mapped file
  mm_close(&tbl->mm_region);
}

uint32_t strtable_len(strtable_t *table) {
  // the table metadata stores the length
  return table->metadata->len;
}

// helper function to return the end of the table.
void *end(strtable_t *table) {
  return ((void *)table->metadata) + table->metadata->size;
}

char *add_element(strtable_t *table, const char *str) {
  DEBUG_PRINT("cur elements %d\n", table->metadata->len);

  // compute the offset that the new element will _end_ at. if the table is
  // empty, this will be the end of the file. if the table is nonempty, this
  // will be where the previous elements starts.
  // clang-format off
  uint32_t last_el_start =
      table->metadata->len > 0 ? 
          table->elements[table->metadata->len - 1].offset :
          0;
  // clang-format on

  size_t len = strlen(str) + 1; // len of element includes \0
  // start offset of element; where it will be written
  void *soffset = end(table) - last_el_start - len;

  DEBUG_PRINT("table end: %p\n", end(table));
  DEBUG_PRINT("last el start: %p\n", NULL + last_el_start);
  DEBUG_PRINT("start offset: %p\n", soffset);

  // ensure start offset of element will be past the end of the index.
  if (soffset < (void *)&table->elements[table->metadata->len + 1]) {
    DEBUG_PRINT("does not fit; end of elements: %p\n",
                (void *)&table->elements[table->metadata->len + 1]);
    // string doesn't fit!
    return NULL;
  }

  // copy the element to its position in the table.
  strncpy(soffset, str, len);
  // add offset to index and increment index pointer.
  table->elements[table->metadata->len].offset = end(table) - soffset;
  table->metadata->len++;
  DEBUG_PRINT("new elements %d\n", table->metadata->len);

  return soffset;
}

char *get_element(strtable_t *table, unsigned int idx) {
  if (idx >= table->metadata->len) {
    // Invalid index.
    return NULL;
  }

  // return pointer to start of element.
  return end(table) - table->elements[idx].offset;
}

int get_element_len(strtable_t *table, unsigned int idx) {
  if (idx >= table->metadata->len) {
    // Invalid index.
    return -1;
  }

  if (idx == 0) {
    // first element size is offset.
    return table->elements[0].offset;
  }

  // return difference between offsets.
  return table->elements[idx].offset - table->elements[idx - 1].offset;
}
