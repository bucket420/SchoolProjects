CFLAGS=-g -Wall -Werror
LDFLAGS=-lssl -lcrypto
CC=gcc

mystery: mystery.o util.o phases.o phase_consts.o phases_split.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o *.gch mystery
