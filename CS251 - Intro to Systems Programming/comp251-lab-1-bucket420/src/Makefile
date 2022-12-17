CFLAGS=-g -Wall -Werror
CC=gcc

my_shell: lab1.o cwd.o
	$(CC) -g -o $@ $^

environment_test: environment_test.o
	$(CC) -g -o $@ $^

argument_test: argument_test.o
	$(CC) -g -o $@ $^

clean:
	rm -f *.o argument_test environment_test my_shell
