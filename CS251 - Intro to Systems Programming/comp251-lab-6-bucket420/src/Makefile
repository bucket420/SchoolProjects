# make DEFINE=-DDEBUG <target>
DEFINE=
DEBUGGER=-g
CFLAGS=$(DEBUGGER) $(DEFINE) -fPIC -Wall -Werror
CC=gcc
LD_FLAGS=-lpthread

# make SAN=tsan <target>
# make SAN=asan <target>
SAN=
ifeq ($(SAN),tsan)
	CFLAGS+=-fsanitize=thread
else ifeq ($(SAN),asan)
	CFLAGS+=-fsanitize=address
endif


TESTS=
TESTS+=tests/ll_test
TESTS+=tests/tp_test
APPS=
APPS+=apps/ii-main

test-all: $(TESTS)
	$(foreach TEST,$(TESTS), ./$(TEST);)

ii-test: apps/ii-main apps/ii_test.py
	./apps/ii_test.py -w 8192 -p 16 -s 16 -l 100 -n 100 -f 20

tests/ll_test: tests/ll_test.o tests/test_utils.o ll.o
	$(CC) $(CFLAGS) -o $@ $^

tests/tp_test: tests/tp_test.o tests/test_utils.o ll.o thread_pool.o tests/tp_test_utils.o util.h
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS)

apps/ii-main: apps/ii-main.o apps/ii.o util.h map/map.o ll.o thread_pool.o
	$(CC) $(CFLAGS) -o $@ $^ $(LD_FLAGS)

copy-books: utils/rand_books.sh
	./$^
	mkdir idx-output

clean:
	rm -f *.o *.so tests/*.o tests/*.so apps/*.o apps/*.so $(TESTS) $(APPS)
	rm -rf books-input
	rm -rf idx-output
	rm -rf apps/__pycache__
