// Interactive driver program for map type.
//
// Usage is printed upon entry.

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"

#define BUFF_LEN 512
#define MAX_ENTRIES 2048

void usage() {
  printf("\tn <size>            create or reallocate map\n"
         "\tp <key> <value>     map[key] = value\n"
         "\tg <key>             get map[key]\n"
         "\tr <key>             remove map[key]\n"
         "\tf                   free map\n"
         "\tv                   dump map contents (tests apply_fn)\n"
         "\ts                   print map stats\n"
         "\td                   call map debug function\n"
         "\t?                   print this message\n"
         "\te                   exit tester\n");
}

// Keep pointers to vals, since they are not freed upon remove.
char *vals[MAX_ENTRIES];
int n_vals = 0;

void create_or_reallocate(map_t **map, char *buff);
void put(map_t *map, char *buff);
void get(map_t *map, char *buff);
void get(map_t *map, char *buff);
void rem(map_t *map, char *buff);
void dbg(map_t *map);
void fm(map_t **map);
void stats(map_t *map);
void dump(map_t *map);

int main(int argc, char **argv) {
  char buff[BUFF_LEN];
  char cmd;
  map_t *map = NULL;

  printf("Interactive map tester: ? for help");
  usage();
  while (1) {
    printf("> ");
    if (!fgets(buff, BUFF_LEN, stdin)) {
      break;
    }
    cmd = buff[0];
    switch (cmd) {
    case '\n':
      break;
    case 'n':
      // create the map or reallocate it.
      create_or_reallocate(&map, buff + 2);
      break;
    case 'p':
      // put an entry into the map
      put(map, buff + 2);
      break;
    case 'g':
      // get the specified entry from the map
      get(map, buff + 2);
      break;
    case 'r':
      // remove the specified entry from the map
      rem(map, buff + 2);
      break;
    case 'f':
      // free the map
      fm(&map);
      break;
    case 'v':
      // dump the map state by calling map_apply
      dump(map);
      break;
    case 's':
      // get the map stats
      stats(map);
      break;
    case 'd':
      // call the map debug function
      dbg(map);
      break;
    case 'q': // fallthrough intentional
    case 'e':
      return 0;
    case '?': // fallthrough intentional
    default:
      printf("Unknown command %c\n", cmd);
      usage();
    }
    memset(buff, 0, BUFF_LEN);
  }
}

void create_or_reallocate(map_t **map, char *buff) {
  long size;
  errno = 0;
  size = strtol(buff, NULL, 10);

  if (errno) {
    perror("strtol");
    return;
  } else if (size <= 0) {
    printf("size must be nonnegative\n");
    return;
  }

  if (*map) {
    printf("Reallocating map to size %ld...", size);
    map_resize(*map, (uint32_t)size);
  } else {
    printf("Allocating map with size %ld...", size);
    *map = map_create((uint32_t)size);
  }
  printf("done.\n");
}

int chk(map_t *map) {
  if (!map) {
    printf("map does not exist. allocate with n <size>\n");
    return 0;
  }
  return 1;
}

void put(map_t *map, char *buff) {
  if (!chk(map))
    return;
  char *key, *value;
  key = strtok(buff, " \t\r\n");
  value = strtok(NULL, " \t\r\n");
  if (key == NULL || value == NULL) {
    printf("Requires two arguments: p <key> <value>\n");
    return;
  }

  // save value so as to not leak it.
  char *nval = malloc(strlen(value));
  strcpy(nval, value);
  vals[n_vals++] = nval;

  printf("Putting map[%s] -> %s...", key, nval);

  int is_new = map_put(map, key, nval);

  printf("done (is_new = %d)\n", is_new);
}

char *check_key(char *buff) {
  int l = strlen(buff);
  if (!l) {
    printf("Key must be nonempty\n");
    return NULL;
  }
  buff[l - 1] = '\0'; // eat \n
  return buff;
}

void get(map_t *map, char *buff) {
  if (!chk(map))
    return;
  char *key = check_key(buff);
  if (!key) {
    return;
  }
  char *value;
  printf("Getting %s...", key);
  int is_in = map_get(map, key, (void **)&value);
  printf("done; value = %s (exists = %d)\n", value != NULL ? value : "NULL",
         is_in);
}

void rem(map_t *map, char *buff) {
  if (!chk(map))
    return;
  char *key = check_key(buff);
  if (!key) {
    return;
  }
  printf("Removing %s...", key);
  int was_in = map_remove(map, key);
  printf("done (exists = %d)\n", was_in);
}

void dbg(map_t *map) {
  if (!chk(map))
    return;
  printf("Calling map_debug...\n");
  map_debug(map);
  printf("done\n");
}

void fm(map_t **map) {
  if (!chk(*map))
    return;
  printf("Freeing map...");
  map_free(map);
  printf("done; pointer value post-free = %p\n", map);

  // reclaim memory from values.
  for (int i = 0; i < n_vals; i++) {
    free(vals[i]);
  }
  n_vals = 0;
}

void stats(map_t *map) {
  if (!chk(map))
    return;
  printf("Getting map stats...");
  map_metrics_t *metrics = map_metrics(map);
  printf("done; metrics:\n");
  printf("\tsize:\t\t%d\n\tmax_depth:\t%d\n\tnum_entries:\t%d\n",
         metrics->curr_size, metrics->max_depth, metrics->num_entries);
}

void *pr_fn(const char *key, void *value) {
  printf("%15s : %s\n", key, (char *)value);
  return value; // preseve value
}

void dump(map_t *map) {
  if (!chk(map))
    return;
  printf("Calling apply to dump map contents...\n");
  map_apply(map, pr_fn);
  printf("done\n");
}
