#include "mm_util.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

void mm_open(const char *fname, size_t size, mm_region_t *region) {
  region->fd = open(fname, O_RDWR | O_CREAT, 0600);
  assert(region->fd != -1);

  uint32_t tsize = size;
  if (size) {
    // size > 0 -> initial open
    int err = posix_fallocate(region->fd, 0, size);
    assert(!err);
  } else {
    // get file size from stat
    struct stat stat;
    assert(!fstat(region->fd, &stat));
    tsize = stat.st_size;
    DEBUG_PRINT("%s exists with size %lu, opening...\n", fname, size);
  }

  void *base =
      mmap(NULL, tsize, PROT_READ | PROT_WRITE, MAP_SHARED, region->fd, 0);
  assert(base != MAP_FAILED);

  region->start = base;
  region->size = tsize;

  DEBUG_PRINT("table opened at address %p\n", region->start);
}

void mm_close(mm_region_t *region) {
  munmap(region->start, region->size);
  close(region->fd);
}
