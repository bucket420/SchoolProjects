#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ii.h"

#define DEFAULT_WORD_FILTER "apps/oec_word_filter.txt"
#define DEFAULT_MAP_SIZE 4096
#define DEFAULT_PARALLELISM 1
#define DEFAULT_SHARDS 1

void usage(char *arg0) {
  printf("Usage: %s -d <input dir> [-o <output dir>] [-e <extension list>] [-s "
         "<shards>] [-m <map size>] [-p <parallelism>]\n",
         arg0);
  printf("Description: Builds and optionally outputs an inverted index of a "
         "text corpus.\n");
  // clang-format off
  printf("Arguments: \n"
      "   -d <input dir>          directory to scan for input files\n"
      "   -o <output dir>         directory to write output index into (optional)\n"
      "   -e <extension list>     list of file extensions to read (defaults to '.txt')\n"
      "   -f <word filter file>   file containing words to filter, one per line\n"
      "   -s <shards>             number of output shards for the index (defaults to 1)\n"
      "   -m <map size>           change number of entries in hash table backing the index (default 1)\n"
      "   -p <parallelism>        max number of threads (default 1)\n");
  // clang-format on
}

int main(int argc, char **argv) {
  char *dir = NULL;
  char *outdir = NULL;
  char *filter_fname = DEFAULT_WORD_FILTER;
  char **extensions = NULL;
  int map_size = DEFAULT_MAP_SIZE;
  int parallelism = DEFAULT_PARALLELISM;
  int shards = DEFAULT_SHARDS;
  int n_ext;
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "d:e:m:p:o:s:h")) != -1) {
    switch (c) {
    case 'd':
      dir = optarg;
      break;
    case 'o':
      outdir = optarg;
      break;
    case 'f':
      filter_fname = optarg;
      break;
    case 'e':
      n_ext = 0;
      extensions = realloc(extensions, (n_ext + 1) * sizeof(char *));
      extensions[n_ext] = strtok(optarg, ",");
      if (!extensions[n_ext]) {
        printf("Must supply at least one argument to -e.\n");
        exit(1);
      }
      while (extensions[n_ext]) {
        n_ext++;
        extensions = realloc(extensions, (n_ext + 1) * sizeof(char *));
        extensions[n_ext] = strtok(NULL, ",");
      }
      break;
    case 's':
      errno = 0;
      shards = strtol(optarg, NULL, 10);
      if (errno != 0) {
        perror("strtol");
        exit(1);
      }
      break;
    case 'm':
      errno = 0;
      map_size = strtol(optarg, NULL, 10);
      if (errno != 0) {
        perror("strtol");
        exit(1);
      }
      break;
    case 'p':
      errno = 0;
      parallelism = strtol(optarg, NULL, 10);
      if (errno != 0) {
        perror("strtol");
        exit(1);
      }
      break;
    case 'h':
      usage(argv[0]);
      exit(1);
    case '?':
      if (optopt == 'c') {
        printf("Option -%c requires a directory name.\n", optopt);
      } else if (optopt == 'e') {
        printf("Option -%c requires a list of extensions.\n", optopt);
      } else if (isprint(optopt)) {
        printf("Unknown option `-%c'.\n", optopt);
      } else {
        printf("Unknown option character `\\x%x'.\n", optopt);
      }
      usage(argv[0]);
      exit(1);
    default:
      usage(argv[0]);
      exit(1);
    }
  }
  if (dir == NULL) {
    printf("Option -d is required.\n");
    usage(argv[0]);
    exit(1);
  }

  char **filter_list = NULL;
  if (filter_fname) {
    printf("> Building filter list from %s\n", filter_fname);
    filter_list = calloc(128, sizeof(char *));
    int filter_len = 128;
    int filter_idx = 0;
    char buff[80];
    FILE *f = fopen(filter_fname, "r");
    while (fgets(buff, 80, f) != NULL) {
      buff[strlen(buff) - 1] = '\0';
      if (filter_idx >= filter_len) {
        int newlen = filter_len * 2;
        filter_list = realloc(filter_list, sizeof(char *) * newlen);
      }
      filter_list[filter_idx] = malloc(strlen(buff) + 1);
      strcpy(filter_list[filter_idx], buff);
      // ensure list is NULL-terminated
      filter_list[filter_idx + 1] = NULL;
      filter_idx++;
    }
    printf("> Read %d words into filter list.\n", filter_idx);
  }

  if (extensions == NULL) {
    extensions = TEXT_EXTENSIONS;
  }
  char **files = list_files(dir, extensions);
  free(extensions);

  printf("Creating ii with files:\n");
  for (char **file = files; *file; file++) {
    printf("> %s\n", *file);
  }

  build_ii(files, filter_list, parallelism, map_size);

  if (outdir != NULL) {
    dump_ii(outdir, shards, parallelism);
  }

  free_ii();
  for (char **file = files; *file; file++) {
    free(*file);
  }
  free(files);
  for (char **filter = filter_list; *filter; filter++) {
    free(*filter);
  }
  free(filter_list);
}
