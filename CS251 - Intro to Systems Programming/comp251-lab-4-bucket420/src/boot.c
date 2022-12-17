#include "boot.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dyn.h"

#include "block_list.h"
#include "disk_array.h"
#include "strtable.h"

struct boot_params paths;

// PHASE ONE
// Load neural network parameters from database that is stored as a
// disk-backed array (disk_array_t in darray.h).
void load_params() {
  const char *load_msg = "[    0.000000]   LOADING WEIGHTS/BIASES [%p]\n";
  const char *load_item = "[    0.000000]     param encoding %lx\n";

  disk_array_t params; // neuron weights/biases, a disk array

  // Open disk array.
  array_open(paths.params_path, 0, sizeof(uint64_t), &params);

  // Print memory mapped region base address.
  printf(load_msg, params.mm_region.start);

  // Cast as array type.
  uint64_t *arr = (uint64_t *)params.array;

  // Read parameters.
  uint64_t item = 0;
  for (int i = 0; i < *params.n; i++) {
    item = arr[i];
    if (i % 100 == 0) {
      // print every 100 params
      printf(load_item, item);
    }
  }

  // Close params database.
  array_close(&params);
}

// PHASE TWO
// Load navigation database from string table (strtable_t in strtable.h).
void load_db() {
  const char *load_msg = "[    0.059309] LOADING [%p]\n";
  const char *load_item = "[    0.059550]     NAV[%d] %s\n";

  strtable_t nav_db; // the navigation database, a strtable

  char *buff = NULL; // tmp buffer for string data
  char *tmp = NULL;  // pointer to current element

  // Open the string table.
  strtable_open(paths.db_path, 0, &nav_db);

  // Print memory mapped region base address.
  printf(load_msg, nav_db.mm_region.start);

  int el_len = 0;
  // Read 10% of table.
  int len = strtable_len(&nav_db);
  int skip = (int)(len / (len * 0.1));
  for (int i = 0; i < len; i += skip) {
    tmp = get_element(&nav_db, i);
    el_len = get_element_len(&nav_db, i);

    // element length should be the length of the string...
    assert(el_len == strlen(tmp) + 1);

    // copy element so that we can mutate it safely.
    buff = malloc(el_len);
    strcpy(buff, tmp);

    // Format output -- find last occurrance of ';' in string and only print
    // this suffix. For example:
    //   Suppose buff is "abc;def;ghi;jkl"
    //   Offset will point to the last ';' --  ";jkl"
    char *offset = strrchr(buff, ';');
    assert(offset);
    //   Offset + 1 is the string "jkl"
    offset++;
    printf(load_item, i, offset);

    free(buff);
  }

  // Close string table.
  strtable_close(&nav_db);
}

// PHASE THREE
// Validate navigation database by reading each element in the table.
void validate_db() {
  const char *load_msg = "[    0.620017] NAV: VALIDATING [%p]";
  const char *load_item = "\n[    0.620017]     NAV [%d]: ";

  strtable_t nav_db; // the navigation database, a strtable

  int el_len = 0;
  char *cur = NULL;

  // Open the string table.
  strtable_open(paths.db_path, 0, &nav_db);

  // Print memory mapped region base address.
  printf(load_msg, nav_db.mm_region.start);

  // Read each element.
  int print_every = 32;
  int len = strtable_len(&nav_db);
  for (int i = 0; i < len; i++) {
    if (i % print_every == 0) {
      // put a new line periodically
      printf(load_item, i);
    }
    // Read element.
    el_len = get_element_len(&nav_db, i);
    cur = get_element(&nav_db, i);
    // "Validate" element; grab first char.
    assert(el_len == strlen(cur) + 1);
    printf(".");
  }
  printf("\n");

  // Close string table.
  strtable_close(&nav_db);
}

// PHASE FOUR
// Load flight log from block list file (block_list.h).
void load_log() {
  const char *load_msg = "[    0.990733] HISTORY: LOADING [%p]\n"
                         "[    0.990733] HISTORY: seeking..\n";
  const char *load_last = "[    0.990733] HISTORY: last location - %s\n";

  block_list_t flight_log; // the flight log, a block list

  // Open log and print memory mapped region base address.
  bl_open(paths.log_path, 0, &flight_log);
  printf(load_msg, flight_log.mm_region.start);

  // Seek to last entry by using bl_prev to get the last element.
  // NOTE:
  //  Using bl_prev initializes the block list tail pointer and navigates the
  //  entire list from the head in order to find the tail.
  uint32_t cur_size = 0;
  char *last = bl_prev(NULL, &cur_size, &flight_log);
  printf(load_last, last);

  // Close list
  bl_close(&flight_log);
}

// PHASE FIVE
// Load flight log, first reading elements in order, and then reverse order.
void renav_log() {
  const char *load_msg = "[    1.003915] HISTORY: replay [%p]\n";
  const char *load_item = "[    1.003915] HISTORY: %s\n";

  block_list_t flight_log; // the flight log, a block list

  // Open log and print memory mapped region base address.
  bl_open(paths.log_path, 0, &flight_log);
  printf(load_msg, flight_log.mm_region.start);

  // Seek to last entry by reading each element of the log in order.
  // NOTE:
  //   This does *not* initialize the tail pointer, since we are reading in
  //   order to find the last element.
  uint32_t cur_size = 0;
  char *cur = bl_next(NULL, &cur_size, &flight_log);
  char *next = bl_next(cur, &cur_size, &flight_log);
  printf(load_item, cur);
  while (next) {
    cur = next;
    printf(load_item, cur);
    next = bl_next(cur, &cur_size, &flight_log);
  }

  printf("[    1.003915] HISTORY: reverse replay\n");

  // Now navigate the list in reverse, starting from where we are now.
  char *prev = bl_prev(cur, &cur_size, &flight_log);
  while (prev) {
    cur = prev;
    printf(load_item, cur);
    prev = bl_prev(cur, &cur_size, &flight_log);
  }

  // Close list
  bl_close(&flight_log);
}

void io(int do_io, int skip) {
  static int c = -1;
  c++;
  if (do_io && (!skip || c > 0)) {
    call(c, paths.dynlib_path);
  }
}

// Execute boot sequence.
void boot(struct boot_params boot_params, int quiet) {
  int do_io = !(quiet & QUIET_SKIP_IO);
  int skip = (quiet & QUIET_SKIP_INTRO);
  paths = boot_params;

  io(do_io, skip);
  io(do_io, skip);

  // Load neural weights
  load_params();
  io(do_io, skip);

  // Load nav db
  load_db();
  io(do_io, skip);

  // Validate nav db
  validate_db();
  io(do_io, skip);

  // Load flight log last entry
  load_log();
  io(do_io, skip);

  // Play and rewind log
  renav_log();
  io(do_io, skip);
}
