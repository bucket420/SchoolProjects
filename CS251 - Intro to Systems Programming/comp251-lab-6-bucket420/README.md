# Lab 6: Implementing a thread pool

<a name="toc"></a>
<!--ts-->
* [Lab 6: Implementing a thread pool](#lab-6-implementing-a-thread-pool)
* [Introduction](#introduction)
   * [Collaboration Policy](#collaboration-policy)
   * [Cloning the lab and getting started](#cloning-the-lab-and-getting-started)
   * [Hand in](#hand-in)
* [Introduction to thread pools](#introduction-to-thread-pools)
   * [Use cases](#use-cases)
   * [Example use](#example-use)
* [Code overview and lab structure](#code-overview-and-lab-structure)
   * [Thread pool state](#thread-pool-state)
   * [Controlling the pool](#controlling-the-pool)
   * [Adding work items](#adding-work-items)
   * [Synchronization](#synchronization)
   * [Implementation details](#implementation-details)
      * [Linked lists](#linked-lists)
      * [threadpool_add](#threadpool_add)
      * [threadpool_counters](#threadpool_counters)
   * [Unit tests](#unit-tests)
      * [Testing with sanitizers](#testing-with-sanitizers)
* [Assignment specifics](#assignment-specifics)
* [General tips](#general-tips)
* [Use case: building an inverted index](#use-case-building-an-inverted-index)
* [Extras](#extras)
   * [Other unit tests](#other-unit-tests)
   * [Example end-to-end test](#example-end-to-end-test)
   * [Project Gutenberg](#project-gutenberg)
* [Wrapping up and handing it in](#wrapping-up-and-handing-it-in)
* [Additional challenges](#additional-challenges)

<!-- Created by https://github.com/ekalinin/github-markdown-toc -->
<!-- Added by: langm, at: Wed Nov 30 03:41:23 UTC 2022 -->

<!--te-->

# Introduction

In this lab you will implement a _thread pool_ library. Your library will be
able to be used by clients to perform tasks in parallel without the client
having to worry about creating, coordinating, or destroying threads.
Additionally, they do not have to worry about some maximum parallelism threshold
being exceeded (i.e., they don't have to worry about creating too many threads).

The lab comes with an example use case: a library and program that can be used
to create and save to file an _inverted index_--a data structure used by search
engines in order to provide fast full-text search.

## Collaboration Policy

It is fine to collaborate for this project. However, all code that you turn in
must be your own. You may not copy code directly from others or from the
internet. Copying code and renaming variables _is copying code._ 

You can and should talk about general problems in the lab, how to use specific
functions, etc.

__When you make your PR, note who you worked with on this lab.__  

## Cloning the lab and getting started

When you clone the lab, make sure that the first thing that you do is create a
new branch to work in!

```
$ git clone <your-repo-url>
$ cd <your-repo>
$ git checkout -b <your-branch-name>
```

## Hand in

Your edits will be limited to `thread_pool.c`. You will also run the inverted
index code and perform a query against it later in the lab. These two files
should be committed and part of your pull request to tun in the lab.

🔝 [back to top](#toc)


# Introduction to thread pools

A [Thread pool](https://en.wikipedia.org/wiki/Thread_pool) is an abstraction
used by programs that have a pattern of behavior where some number of pieces of
work are generated, and each work item can be processed independently.

These types of programs--where there is no dependency between discrete pieces
of work--benefit from parallelization. Since this is a common pattern (we will
discuss use-cases below), there is a need for a general solution. 

A thread pool is one such solution. A thread pool is an object that
independently manages a collection of _worker threads_. It allows a user to add
_work items_ to queues, and schedules work from that queue to be run by the
threads that it manages.

Therefore, from the user's perspective, a thread pool provides parallelism in an
opaque way:

1. The user creates the thread pool, specifying a maximum number of concurrent
   threads that should be working. They then start the thread pool, which causes
   the pool to create some number of worker threads..
2. When the user wants work to be done by one of the threads, they call a
   function `addWork()` to add some work to the thread pool's work queue.
3. If there is an idle worker thread, the thread pool schedules that work to be
   done right away by the worker thread. If all worker threads are busy doing
   work, the work is enqueued. 
4. When the user is done processing work, they stop and destroy the pool.

This sort of object has many benefits:

* A programmer with a problem that can benefit from parallelization and is
  amenable to the thread pool model does not have to worry about writing
  (perhaps complex) code to manage threads and their synchronization.

* Since there is some overhead to creating and destroying threads, this model
  can be more efficient than a model where threads are started on-demand, when
  work is generated.

* The number of worker threads--and therefore the amount of parallelism--is
  controlled by a single object, and changes to this parameter only affect the
  construction of the pool, rather than requiring a programmer to reason about
  the current degree of parallelism when work is generated.

## Use cases

Thread pools are generally useful for so-called [embarassingly-parallel
programs](https://en.wikipedia.org/wiki/Embarrassingly_parallel), as well as for
other use cases where work can be done independently, but there is a need to
constrain parallelism.

Embarassingly-parallel problems are those where there is a large amout of work,
but each piece of work can be done independently. Examples include brute-force
searches, independent processing of input files (this is the use-case for this
lab), image processing, some matrix operations, etc.

Problems were parallelism should be constrained and thread pools are a useful
abstraction are in server programs.
[RPC (remote procedure
call)](https://en.wikipedia.org/wiki/Remote_procedure_call) libraries are one
example, where a server may service many requests to execute a function from
many different clients. A related and important use-case is the implementation
of a web server, where independent HTTP requests can be handled by independent
threads. 

In both of these cases, it is important to limit the amount of clients that are
admitted a connection so as to not overwhelm the server. A thread pool solves
this problem by making client connections the work items.

## Example use

Search engines use a software system called a [web
crawler](https://en.wikipedia.org/wiki/Web_crawler) to scrape and collect an
archive of web pages and their link structures.  They then analyze the contents
of pages to create an index of the web that is used to populate search engine
results.

Google provides a [nice
overview](https://www.google.com/search/howsearchworks/how-search-works/organizing-information/)
aimed at lay people of how search works.

The general structure of a web crawler is as follows:

* Fetch the text of a URL/web page and archive it.
* Parse the fetched text and discover links to URLs of other web pages. 
  * For every URL that has not yet been visited, repeat the previous two steps.

The process terminates when the crawler does not discover any more unvisited
links. If you've taken a data structures and algorithms course, you might see
how this algorithm is similar to a search of a graph. 

You may also see how this algorithm could easily be parallelized: fetching and
parsing an individual web page is an independent task and could be parallelized
with fetching and parsing of other web pages.

This is a perfect algorithm for parallelization using a thread pool! A work item
for the thread pool would be an unvisited URL. The program would start with a
work queue of initial URLs (initial web pages to fetch).

Worker threads would dequeue URLs to visit, fetch the URL, and then enqueue
newly discovered URLs in the work queue.

It would be desirable to limit parallelism in this algorithm for many reasons:
the network bandwidth of the host, disk bandwidth available for storing fetched
pages, work required for parsing a page, etc.

🔝 [back to top](#toc)

# Code overview and lab structure

```
lab 6
|-- doc ........................... images used in this file
|-- README.md ..................... this document
`-- src ........................... main source code for the lab
    |-- apps 
    |   |-- ii.c .................. main inverted index source
    |   |-- ii.h .................. main inverted index header
    |   |-- ii-main.c ............. inverted index main function
    |   |-- ii_query.py ........... tool to query an inverted index
    |   |-- ii_test.py ............ inverted index end-to-end test
    |   `-- oec_word_filter.txt ... word filter (top 100 english words)
    |-- books-input                 
    |   `-- ....................... input files for inverted index
    |-- idx-output
    |   `-- ....................... output files for inverted index
    |-- ll.c ...................... linked list library for queueing work
    |-- ll.h ...................... linked list header
    |-- Makefile .................. make targets
    |-- map ....................... map from lab 3
    |   |-- map.h ................. header file to include to use map
    |   `-- map.o ................. prebuilt thread safe solution to lab 3
    |-- tests 
    |   |-- ll_test.c ............. linked list unit test
    |   |-- test_utils.c .......... unit testing utilities
    |   |-- test_utils.h .......... unit testing utilities header file
    |   |-- tp_test.c ............. thread pool unit test
    |   |-- tp_test_utils.c ....... utils for thread pool test
    |   `-- tp_test_utils.h ....... utils for thread pool test header
    |-- thread_pool.c ............. main source file implementing a thread pool
    |-- thread_pool.h ............. header file for thread pool
    |-- util.h .................... misc asserts and other macros
    `-- utils 
        |-- process-books.py ...... script that processed Gutenberg archive
        `-- rand_books.sh ......... script to chose Xmb of random books for testing
```

## `Make` targets

The main `make` target for the lab builds and runs a collection of unit tests.
You can use this to test your code as you are building your thread pool.

* Default target/`test-all`: running `make` or `make test-all` will build and
  run the unit tests for the lab. These unit tests should cover quite a bit of
  functionality of the thread pool, and should be run as you complete features.
  Your final implementation should pass all tests.

  Note that if you discover bugs in your thread pool when you use the inverted
  index code, you should try to add a test that would've exposed this bug. This
  is good software engineering practice.

* `clean`: running `make clean` will clean up all intermediate files used by the
  lab, including compiled files, binaries, as well as the extra files created
  for the inverted index.

The other build targets are related to the inverted index application (see
section below on how to build + run the inverted index).

* `copy-books`: this creates the input files that you will use to build your
  inverted index, as well as the output directory that will store the index.

  `make copy-books` creates two directories in your `src` directory:
  `src/books-input` and `src/idx-output`. The input directory is filled with
  ~100MB of plaintext books randomly chosen from [Project
  Gutenberg](https://www.gutenberg.org/) (the entire Project Gutenberg corpus is
  in `/opt/gutenberg`.

* `apps/ii-main`: this target builds in the inverted index application. `make
  apps/ii-main` will produce the `ii-main` binary in `apps/` that you can run to
  analyze your reduced-size copy of the corpus.

  See below for more information on running the inverted index application.

🔝 [back to top](#toc)

# `thread_pool.h` overview

The thread pool abstraction in this lab is defined in `thread_pool.h`. This
describes the thread pool's behavior and functions that can be used to
manipulate it.

__Important:__ The header file is an extremely important piece of documentation
for this lab. It is correspondingly important that you thoroughly read the
header file and understand what the thread pool is supposed to do.

## Thread pool state

The thread pool is in one of three states: _stopped_, _running_, _stopping_, or
_draining_. The initial state is _stopped_ and the user of the thread pool
can start it. When the thread pool is _running_, worker threads will dequeue
work items from an internal work queue and process them. The thread pool user
can continue to add work to the work pool as the pool is _running_.

When the user wants to stop the pool, they can do so in one of two ways:

* By completing all in-flight work, but not processing addition work. When the
  thread pool is in this state, it is _stopping_.
* By completing all in-flight _and currently queued work__. When the thread pool
  is in this state, it is _draining_.

The thread pool maintains a secondary queue that allows items to be enqueued
while the pool is _draining_ or _stopping_.

## Controlling the pool

The user transitions the pool between states by calling `threadpool_start` and
`threadpool_stop`.

Thread pools are created with calls to `threadpool_create` and
`threadpool_destroy`.

## Adding work items

A work item for the thread pool is a struct with three fields:

* `fn` -- a function that is called by the worker thread to do the work. You can
  think of this function as being similar to the `start_routine` parameter for
  `pthread_create`. `fn` accepts a single `void*` parameter and may return a
  `void*` parameter as well.
* `work` -- a pointer to an argument for `fn`. When a worker thread dequeues
  the work item, this is the parameter that is passed to `fn`. This is useful
  when worker threads all do similar work, but the work is parameterized. For
  example, in a web crawler, each worker thread does identical work: fetch,
  archive, and parse a URL. In this case, the `fn` is is the fetch/archive/parse
  algorithm and the `work` is the URL.
* `cb` -- a callback function that is invoked after `fn` completes. This
  function is passed the return value of `fn`.

You can think of the execution of a work item as being the chained function
call `cb(fn(work))`.

Note that `cb` and `work` are optional. If they are not required, they may be
`NULL`.

## Synchronization

All functions in the thread pool are able to be called by potentially-many
client threads, and therefore must protect against race conditions on the thread
pool state. 

Additionally, as worker threads will be dequeuing work concurrent to each other
_and_ to user enqueue operations, worker threads must synchronize among
themselves, as well.

## Implementation details

The thread pool struct is defined in `thread_pool.c`. It has the following
fields:

* `state` -- the current state (`STOPPED`, `RUNNING`, `STOPPING`, `DRAINING`) of
  the thread pool.
* `config` -- a copy of the thread pool config.
* `mu` -- a mutex that guards all variables of the thread pool. Any concurrent
  access to any variable in the thread pool should be guarded by this mutex.
* `avail` -- a condition variable used to activate idle threads. When work is
  unavailable, a worker thread can wait on the condition variable. When work
  arrives, the condition variable can be signalled in order to wake any waiting
  worker.
* `threads` -- the array of worker thread ids/handles.
* `targs` -- an array of worker thread arguments. This allows the thread pool to
  give workers an id as well as a reference to the thread pool itself.
* `counters` -- some telemetry counters that users can retrieve in order to see
  the state of work execution in the pool.
* `queued_work` -- the main work queue. Worker threads pull work from this
  queue.
* `paused_work` -- work that is queued while the pool is not _running_. When the
  pool is either stopped or draining, this queue is the one that stores enqueued
  work. When the pool is started, the `paused_work` queue is appended to the
  main work queue.

### Linked lists

Work queues in this thread pool implementation are linked lists. A bare-bones
linked list implementation has been provided for you. See `ll.h` for its
documentation and `ll.c` for its implementation. Enqueuing/dequeueing work for
the thread pool will ultimately result in operations on the `queued_work` or
`paused_work` lists.

Note that the linked list data structure is __not__ thread safe. That means that
any potentially-concurrent access to the linked list must be synchronized!

### `threadpool_create` and  `threadpool_destroy`

These two functions initialize and tear down the thread pool, respectively.

* `threadpool_create` allocates the `struct threadpool` pointer and all fields
  in the pool and sets their initial value. Additionally, it calls
  `pthread_mutex_init` and `pthread_cond_init` to initialize the mutex/condition
  variable. 

  Notice that `threadpool_t` is really a pointer to a `struct
  threadpool` (see `thread_pool.h`). This makes the type opaque to the user;
  they don't ever have to deal with knowing that the returned pool is a pointer
  to a struct.

* `threadpool_destroy` frees the pool and work queues. Notice that `ll_free` can
  be used to free a linked list.

### `threadpool_start` and `threadpool_stop`

These functions control the state of the thread pool.

* `threadpool_start` should create all of the worker threads for the pool and
  modify its state to `RUNNING`. Note that it should only do so if the pool is
  `STOPPED`.

  Since work may be in the `paused_work` queue, this work should also be joined
  with the `queued_work` queue.

* `threadpool_stop` should signal that all threads should complete (either by
  draining the queue or by simply finishing in-flight work, depending on which
  value the user supplied to `options`). 

  Once these threads finish the work, they should be joined and the thread pool
  state should become `STOPPED`.

  Note that this function is a _blocking_ function: it does not return until all
  threads have been stopped. This means that if a user calls `threadpool_stop`
  they are guaranteed that all in-flight/queued work has been completed when the
  function returns.

  Since threads may be waiting for work when `threadpool_stop` is called (e.g.,
  if the queue is empty), they may need to be woken up by signalling the `avail`
  condition variable.

### `threadpool_add`

This function adds work to the thread pool. The function can be called when the
pool is in any state; depending on the state of the pool, the work may be added
to different queues.

If threads are waiting for work, it may be necessary to signal them that work
has arrived.

### `threadpool_counters`

This function returns a snapshot of the current counter values. Since the
counters may be modified by different threads as work is queued/dequeued,
modification and copying of the counters must be protected by the mutex.

## Unit tests

This lab has a set of unit tests that exercise various behaviors of the thread
pool. These tests are located in `src/tests/tp_test.c` and use other utilities
defined in the `src/tests/` directory.

If a test fails, you should determine what the test is checking for by reading
the test source code. `gdb` can be used to run tests. To run tests manually:

```
$ cd src
$ make tests/tp_test
$ ./tests/tp_test
$ gdb tests/tp_test
```

### Testing with sanitizers

In addition to `valgrind` there are other tools for automatically checking C/C++
programs for memory errors. In this lab we will use another set of tools
supported by the C compiler: santizers. Specifically, we will be using
[_ThreadSanitizer_](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
(TSan) and
[_AddressSantizer_](https://github.com/google/sanitizers/wiki/AddressSanitizer)
(ASan).

Both tools work by having the compiler insert instrumentation code when your
program is compiled. Then, when the program is run, this code validates memory
accesses. They both check for different sorts of errors, however:

* ASan checks for memory issues: leaks, double frees, use after free, buffer
  overflows, etc.
* TSan checks for concurrency issues; specifically, it looks for race
  conditions. 

Both tools are supported by the compiler and require no special commands to run
the program with the sanitizer on. However, the instrumentation that they insert
can be costly (e.g., up to 10x slower), so the sanitizers are not turned on by
default. You will have to recompile if you want to use them.

Before turning in the lab, you should make sure that the tests pass when the
sanitizers are turned on.

* Running TSan:

  ```
  $ make clean
  $ make SAN=tsan test-all
  ```

  After the tests pass, you should make sure to `make clean` to remove the TSan
  instrumentation.

* Running ASan:

  ```
  $ make clean
  $ make SAN=asan test-all
  ```

  Again, after the tests pass, you should make sure to `make clean` to remove
  the ASan instrumentation.

🔝 [back to top](#toc)

# Assignment specifics

Almost all of the code in this lab has been written for you. The only exception
is two functions: `is_stop` and `get_work`.

* `is_stop` -- a function that determines whether a thread should halt. This
  function is a convenience function that is not strictly necessary to
  implement, but will make the implementation of `get_work` much cleaner. Read
  the documentation of the function; the comments describe what the function
  needs to do.

  Note that (as written) the function does __not__ require locking the mutex in
  order to read the state of the pool's variables. This allows the function
  implementation to be simpler, but requires that the caller of the function
  (`get_work`) ensure that the mutex is locked whenever it calls `is_stop`.

  Also note that this is simply a suggestion: you are free to implement this
  however you choose.

* `get_work` -- a function called by a worker thread in order to get work from a
  pool. Like `is_stop`, this function's behavior is documented in a comment.
  This function needs to lock the mutex in order to get queued work from the
  queue and to read the state of the pool.

  As described, this function should use the `avail` condition variable to wait
  until work is available to execute.

  If you are worried about how to fill in the work parameter, look at the
  documentation for `ll_take`.

# General tips

* GDB supports debugging multithreaded programs. The only difference between
  debugging a multithreaded program and a single threaded program is that each
  thread has a different stack and local state. GDB supports switching between
  threads with a handful of commands:

  * `info threads` -- this shows all threads associated with a program.
  * `thread [n]` -- this allows switching between threads.

  Once a thread is selected within GDB, all of the usual commands are available
  to you. Commands that inspect local variables or the call stack are all within
  the context of the currently-selected thread.

* Running the tests with sanitizers can help detect memory errors and race
  conditions. These are invaluable in helping you narrow down the source of bugs
  when there is concurrency in a program and the execution order of statements
  is either not known or difficult to reason about.

* Do not read or write shared variables without synchronization. In this lab, a
  thread should _always_ hold a mutex when reading or modifying any data shared
  with another thread. When a variable is only read from/written to while a
  mutex is held, the variable is _protected by_ the mutex.

* Threads can be in _many_ possible states. It is extremely important to try to
  limit the number of states that you're thinking about by appealing to the
  knowledge that when a mutex is held, the variables that are protected by that
  mutex cannot be changed by other threads, nor can intermediate state be read
  by other threads. 

* Remember that when reasoning about concurrency you need to think
  _assertionally_ rather than _operationally_. If you catch yourself thinking
  about the order of operations between different threads, you should take a
  step back and think about what you _know_ to be true about ordering of events
  based on _known synchronization points_. 

🔝 [back to top](#toc)

# Use case: building an inverted index

An _inverted index_ is a map-like data structure that maps words to the
locations where they appear. Inverted indices are used by search engines in
order to provide fast search results--instead of performing a full-text search,
the search engine simply looks up search terms in the index to generate results.

You can think of an inverted index like an index in the back of a book: to
determine where a word can appear, to you look up the word in the index and find
a list of page numbers. The only difference is that an inverted index is
typically built from multiple documents: the entry for a word will be a list of
documents and the locations within those documents where the word appears.

The [Wikipedia page](https://en.wikipedia.org/wiki/Inverted_index) for an
inverted index is helpful for more information.

You have been given an example application that uses your thread pool to build
an inverted index from a set of books.

The source code for the inverted index application is in `src/apps/`. There are
three main files:

* `apps/ii-main.c` -- the file containing the `main` function for building the
  inverted index.
* `apps/ii.h` and `apps/ii.c` -- the header and source file for the code that
  constructs the inverted index (and optionally writes it out to a set of
  files).

The inverted index application takes several flags:

```
Usage: .//apps/ii-main -d <input dir> [-o <output dir>] [-e <extension list>] [-s <shards>] [-m <map size>] [-p <parallelism>]
Description: Builds and optionally outputs an inverted index of a text corpus.
Arguments: 
   -d <input dir>          directory to scan for input files
   -o <output dir>         directory to write output index into (optional)
   -e <extension list>     list of file extensions to read (defaults to '.txt')
   -f <word filter file>   file containing words to filter, one per line
   -s <shards>             number of output shards for the index (defaults to 1)
   -m <map size>           change number of entries in hash table backing the index (default 1)
   -p <parallelism>        max number of threads (default 1)
```

We will use it to create an inverted index of a random selection of books from
Project Gutenberg. To create the list of books, run `make copy-books`. This runs
a shell script that copies a random set of books to the directory
`src/books-input`. 

It should copy about 300MB of books to `books-input`. Depending on the size of
the books that were randomly selected, this should be several hundred books. You
can verify this after the command completes:

```
$ make copy-books
$ du -sh books-input
$ ls books-input | wc -l
```

The inverted index that is created by the `ii-main` program is written out to
disk. The make target that copies the books creates the output folder that the
program will use. To run the program on the input and generate the output files,
run:

```
$ make apps/ii-main
$ ./apps/ii-main ./apps/ii-main -d books-input -e .txt -p 32 -o idx-output -s 256
```

This creates 256 output
[_shards_](https://en.wikipedia.org/wiki/Shard_(database_architecture)) and uses
32 threads to create the index. The index output is sharded as the number of
words and their occurrences is rather large.

Sharding is the act of dividing up a set of data into physical partitions (in
our case, files). This can improve search performance by splitting reducing the
number of records that need to be searched for a given query. A real-world
analogy would be a set of books in an encyclopedia: to find the entry for
"Spain" you would first go to the book containing the "S" words, and then look
for "Spain" in the book. Although does anyone use encyclopedias anymore? 

To query the inverted index, you can use the `ii-query` python script. This
takes two flags: 

* `-i <directory>` -- the folder containing the inverted index files.
* `-w <words>` -- a comma-separated list of words to search the index for.

After the index is built, try querying it for some words!

```
$ ./apps/ii_query.py -i idx-out -w hello,world,bubble,scrub,friend
```

# Extras

I included some files that were used to create the scaffolding for this lab. If
you are curious, you can poke around at some of the source that is distributed
with the lab.

## Other unit tests

There are unit tests for the linked list implementation in `tests/ll_test.c`.
There is also a _very_ basic unit testing framework in `tests/test_utils.h` and
`tests/test_utils.c`. In future semesters, all labs will have richer unit tests.

## Example end-to-end test

This lab also contains an example end-to-end test that was used to verify the
correctness of the inverted index program. If you're curious, you can look at
`src/apps/ii_test.py`.

You can also try running the test with something like the following:

```
$ ./apps/ii_test.py -w 1024 -p 16 -s 16 -l 100 -n 100 -f 10
```

## Project Gutenberg

The entire Project Gutenberg corpus is in `/opt/gutenberg`. Files are plain
text.

The files were downloaded respecting Project Gutenberg's [instructions for users who wish to do so](https://www.gutenberg.org/policy/robot_access.html):

```
wget -w 2 -m -H "http://www.gutenberg.org/robot/harvest?filetypes[]=txt&langs[]=en"
```

Files were then processed with the Python script in `utils/process-books.py`,
which unzipped each book and attempted to rename the text file with the title of
the book.

The downloaded archive of zipped files was 11GB and is archived as
`/opt/gutenberg/gutenberg.tgz`. The unzipped and deduplicated text files are
17GB and are in `/opt/gutenberg/txt`. 

🔝 [back to top](#toc)

# Wrapping up and handing it in

When you have completed your thread pool implementation and wish to turn in your
code, commit your changes to your branch and create a pull request with you
completed lab:

```
$ git add src/thread_pool.c
$ git commit -m '<your commit message>'
$ git push
```

Make sure to follow the instructions to create a pull request for me to review.

__Congratulations!__ This is another meaningful and subtle piece of code that
you've completed!

🔝 [back to top](#toc)

# Additional challenges

If you found this lab interesting and would like to build on your thread pool
implementation, here's a few suggestions for how you might expand on
the codebase and do some experiments:

* Building an inverted index an IO-bound operation: it the throughput of the
  algorithm is really constrained by disk throughput. Try running the inverted
  index program with different numbers of threads/shards. How does the speedup
  of the program vary as you vary these parameters? To what degree does the fact
  that the program is really bound by the speed of disk IO affect how the
  program scales as more threads are added? 

  Note that you can use the `time` program to time the runtime of another
  program. For example:

  ```
  $ time ./apps/ii-main ...
  ```

* The interface of the `map` from lab 3 had to be updated in order for the
  inverted index application to be able to use it. Take a look at `map/map.h`.

  * A new function (`map_get_or_put`) had to be added. Why do you think this
    operation was now necessary in a multithreaded environment?

  * The map is also now thread safe. The header file has a note on how the map
    was made thread safe. Why do you think that I chose to implement the map
    with one mutex per row rather than a global mutex?

  * Make your map thread safe. You can replace my implementation (`map/map.o`)
    with your own `.o` file if you compile lab 3.

* Create a different, CPU-bound use of the thread pool. For example, you might
  try to use such a pool to do something like invert a hash function. There is a
  dictionary of English words in `/usr/share/dict`.

* Create a different method of stopping a thread pool -- use `pthread_cancel`
  and implement a new option for `threadpool_stop` (e.g.,
  `THREADPOOL_STOP_KILL` that interrupts in-progress work).

* Implement a way for a user to track the status of a work item. For example,
  you might make `threadpool_add` return a work item identifier. Then, you might
  make a new function `threadpool_work_status(work_item_id)` that returns the
  state of the item (whether it is queued, processing, unknown, complete, etc.).

🔝 [back to top](#toc)

