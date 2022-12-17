#ifndef __II_H__
#define __II_H__

// Default extensions for text files. Use as a default argument for list_files.
extern char *TEXT_EXTENSIONS[];

// Build an inverted index by processing words in parallel.
//
// Files should be an array of paths to files to scan; filter is a disallow list
// of words to ignore (e.g., and, of, the, ...). The max_parallelism controls
// how many threads will be used to build the index, and map_size is the initial
// size of the hash table used to store the ii.
void build_ii(char **files, char **filter, int max_parallelism, int map_size);

// Free the inverted index.
void free_ii();

// Output the index to files in the target directory, using the given number of
// shards.
// TODO: provide load functionality
void dump_ii(char *dir, unsigned int shards, int max_parallelism);

// List files in a directory, filtered by a list of extensions. If extensions is
// NULL, lists all files.
char **list_files(char *dir, char **extensions);

#endif
