DEBUGGER=-g
CFLAGS=$(DEBUGGER) -Wall -Werror
CC=gcc
OUTPUT=

all: idriver word_count stress_test

idriver: interactive_driver.o map.o
	$(CC) $(DEBUGGER) -o $@ $^

word_count: word_count.o map.o fnv64.h rand1.h
	$(CC) $(DEBUGGER) -o $@ $^

stress_test: stress_test.o map.o fnv64.h rand1.h
	$(CC) $(DEBUGGER) -o $@ $^

run_word_count: word_count
ifeq ($(OUTPUT),file)
	valgrind --leak-check=full ./word_count -f books/alice-in-wonderland.txt -s 20 -d -r > valgrind-wc-out.txt 2>&1 
else
	valgrind --leak-check=full ./word_count -f books/alice-in-wonderland.txt -s 20 -d -r 
endif

run_stress_test: stress_test 
ifeq ($(OUTPUT),file)
	valgrind --leak-check=full ./stress_test -r 50 -i 4 -s 256 -k 2048 -m 4 > valgrind-st-out.txt 2>&1 
else
	valgrind --leak-check=full ./stress_test -r 50 -i 4 -s 256 -k 2048 -m 4
endif

clean:
	rm -f *.o idriver word_count stress_test
