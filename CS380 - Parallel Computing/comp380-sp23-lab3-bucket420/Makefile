all: lab3 lab3_aos nbody

CC=oshcc
CFLAGS=-Wno-unused-parameter -Wno-format-overflow -Wall -Wextra -g -rdynamic -O3 -msse4.2
#CFLAGS=-fsanitize=address -fsanitize=undefined -Wno-unused-parameter -Wno-format-overflow -Wall -Wextra -Werror -g -rdynamic -O3 -msse4.2

lab3: lab3.o wctimer.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

lab3_aos: lab3_aos.o wctimer.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

nbody: 
	gcc -o nbody nbody.c -O3 -lm

clean:
	rm -f *.o lab3 lab3_aos nbody
