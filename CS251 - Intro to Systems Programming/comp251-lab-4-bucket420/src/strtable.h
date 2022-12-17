#ifndef __STRTABLE_H__
#define __STRTABLE_H__

#include <stdint.h>
#include <stdlib.h>

#include "mm_util.h"

typedef struct strtable_t strtable_t;

// --------------------------------------
// strtable file format and memory layout
// --------------------------------------
//
// strtables are append-only in-memory file-backed databases. strtables have a
// maximum size, which is supplied at create time, but store a variable number
// of strings (which are called elements), depending on size of the stored
// elements.
//
// Typical usage:
//
//    strtable_t table;
//    strtable_open(filename, table_size_bytes, &table);
//
//    // Append an element
//    add_element(&table, string);
//    ...
//    // Get an element and its size
//    char *str = get_element(&table, index);
//    int len = get_element_len(&table, index);
//    ...
//
//    strtable_close(&table);
//
// strtables store _strings_ at _indicies_ and can be thought of as a
// append-only list of strings. Users can append a string to the string table,
// retrieve the string at a given index, and can get information about the
// string at a particular index.
//
// The strtable format looks like the following (description follows):
//         byte | contents      | description
//         -----|---------------|-------------
//            0 | STBL          | identifying marker
//            4 | size          | uint32 size of file
//            8 | n             | uint32 number of elements
//           12 | index[0]      | element 0 metadata
//              | ...           |
//      12 + 4i | index[i]      | element i metadata
//              | ...           |
//              | index[len-1]  | element len-1 metadata
//  12 + 4(len) |               | start of data region
//              |               |
//              | element len-1 | last element added
//              | element len-2 |
//              | ...           |
//              | element 1     |
//              | element 0     | first element added is at end of table
//         size | end           | end of data
//
// The file can be thought of being divided into three sections:
//   1. A file metadata header.
//   2. An index section storing element offsets.
//   3. A data section storing the elements.
//
// The metadata header begins with the four characters STBL -- this identifies
// the file as being a string table file type. Immediately following this header
// at index four is a 4-byte unsigned integer representing the total size of the
// table in bytes (called size), followed by a 4-byte unsigned integer
// representing the number of elements currently stored in the file (called
// len).
//
// The index section stores the offsets of the elements currently stored in the
// table. There are len entries in this index, stored in ascending order (that
// is, the offset for the first element added to the table immediately follows
// the header section). Each index entry is currently only a 32-bit offset that
// represents where the start of the string is in the file AS AN OFFSET FROM THE
// END OF THE FILE.
//
// This means that if an element has an offset of 10 and the file size if 512,
// the element will start at byte 502.
//
// In the data section, elements are stored in reverse order of their index.
// Each element retains its null-terminator. Strings are packed together, so
// the space after the null terminator is either the end of the table OR the
// start of another string.
//
// Note: strtable strings returned by get_element are mutable and changes to the
// string are reflected on disk and for subsequent gets. One can always
// determine the available size for mutations to an element by calling
// get_element_len.

// table metadata struct
struct table_metadata {
  char hdr[4];   // header chars
  uint32_t size; // total size of table
  uint32_t len;  // number of elements
};

// strtable struct
struct strtable_t {
  struct table_metadata *metadata; // pointer to metadata/table start
  struct table_element *elements;  // pointer to elements metadata start
  mm_region_t mm_region;           // memory map info
};

// element metadata
struct table_element {
  uint32_t offset; // currently only storing element offset
};

// Create a strtable.
//
// If table exists, size should be 0. When size is nonzero, the table will be
// created with the given size.
void strtable_open(char *path, uint32_t size, strtable_t *tbl);

// Close a table
//
// Frees tbl.
void strtable_close(strtable_t *tbl);

// Add an element to the table.
//
// Returns pointer to the copy of str in the table, or NULL if the string did
// not fit in the table.
char *add_element(strtable_t *table, const char *str);

// Get the length of the table (in terms of number of elements).
uint32_t strtable_len(strtable_t *table);

// Return the element at index idx. Returns null if index is not in table range.
char *get_element(strtable_t *table, unsigned int idx);

// Return length of element at index idx. Returns -1 if index is not in table
// range.
int get_element_len(strtable_t *table, unsigned int idx);

#endif
