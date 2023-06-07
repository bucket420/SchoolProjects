CC=mpicc
MPICC=mpicc
CFLAGS=-Wno-unused-parameter -Wno-format-overflow -Wall -Wextra -Werror -g -rdynamic -O3 -msse4.2

all: clean pingpong ring barrier-ring barrier-tree

pingong:
	$(MPICC) $(CFLAGS) -o $@ pingpong.c

ring:
	$(MPICC) $(CFLAGS) -o $@ ring.c

barrier-ring:
	$(MPICC) $(CFLAGS) -o $@ barrier-ring.c

barrier-tree:
	$(MPICC) $(CFLAGS) -o $@ barrier-tree.c

clean:
	rm -f *.o pingpong ring barrier-ring barrier-tree
