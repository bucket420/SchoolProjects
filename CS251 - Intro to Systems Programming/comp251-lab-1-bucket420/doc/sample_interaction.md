```
langm@lily [~/251/labs/lab1/src] $ make
gcc -g -Wall -Werror   -c -o lab1.o lab1.c
gcc -g -Wall -Werror   -c -o cwd.o cwd.c
gcc -g -o my_shell lab1.o cwd.o
langm@lily [~/251/labs/lab1/src] $ ./my_shell 
Welcome to MyShell!
Path: 	/home/langm/bin
	/opt/hpc/bin
	/usr/local/Modules/bin
	/usr/local/sbin
	/usr/local/bin
	/usr/sbin
	/usr/bin
	/sbin
	/bin
	/usr/games
	/usr/local/games
	/snap/bin
	/usr/local/go/bin
$ pwd
/home/langm/251/labs/lab1/src
$ cd ..
$ pwd
/home/langm/251/labs/lab1
$ cd src
$ cat Makefile
CFLAGS=-g -Wall -Werror
CC=gcc

my_shell: lab1.o cwd.o
	$(CC) -g -o $@ $^

environment_test: environment_test.o
	$(CC) -g -o $@ $^

clean:
	rm -f *.o environment_test my_shell
$ history
  4: pwd
  3: cd ..
  2: pwd
  1: cd src
  0: cat Makefile
$ !0
cat Makefile
CFLAGS=-g -Wall -Werror
CC=gcc

my_shell: lab1.o cwd.o
	$(CC) -g -o $@ $^

environment_test: environment_test.o
	$(CC) -g -o $@ $^

clean:
	rm -f *.o environment_test my_shell
$ !4
pwd
/home/langm/251/labs/lab1/src
$ make environment_test
gcc -g -Wall -Werror   -c -o environment_test.o environment_test.c
gcc -g -o environment_test environment_test.o
$ ./environment_test
OK 12 /home/langm/bin:/opt/hpc/bin:/usr/local/Modules/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/usr/local/go/bin
$ ls -al
total 92
drwxrwx--- 2 langm langm  4096 Sep  6 17:59 .
drwxrwx--- 4 langm langm  4096 Sep  6 17:57 ..
-rw-rw---- 1 langm langm   143 Sep  5 22:55 cwd.c
-rw-rw---- 1 langm langm    71 Sep  5 22:56 cwd.h
-rw-rw---- 1 langm langm  3640 Sep  6 17:58 cwd.o
-rwxrwx--- 1 langm langm 11240 Sep  6 17:59 environment_test
-rw-rw---- 1 langm langm   485 Sep  5 22:38 environment_test.c
-rw-rw---- 1 langm langm  6896 Sep  6 17:59 environment_test.o
-rw-rw---- 1 langm langm  4398 Sep  5 23:30 lab1.c
-rw-rw---- 1 langm langm 14848 Sep  6 17:58 lab1.o
-rw-rw---- 1 langm langm   176 Sep  6 17:50 Makefile
-rwxrwx--- 1 langm langm 19040 Sep  6 17:58 my_shell
$ exit
langm@lily [~/251/labs/lab1/src] $
```

