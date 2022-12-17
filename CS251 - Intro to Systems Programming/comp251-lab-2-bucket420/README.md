# Lab 2: Debugging C Programs with GDB

## Introduction

For this lab, you will not write any code at all!

Instead, you will be using a debugger to determine what passwords you will need
to enter in order to unlock a puzzle program. 

The program has been given to you as a compiled binary. When you run it using
the debugger and examine the program's memory and source code, you can determine
what to enter at a sequence of five prompts.

The objectives for this lab are the following:

* Become familiar with using the GDB debugger.
* Understand how debuggers can help understand program behavior.
* Use a debugger to help understand complex code.
* Further your understanding of C.

### Collaboration

For this lab, collaboration is OK, __as long as you are sharing strategies for
discovering how to unlock the puzzle.__ You can and should talk about general
problems in the lab, how to use specific functions, etc. You should not be
telling people the specific pass codes, or explaining what exactly the code is
doing.

__When you make your PR, note who you worked with and on what
phases/sections/etc.__ 

## Assignment Overview

Begin by cloning this GitHub repository on lily. Make sure that the first thing
that you do is __create a new branch!__

```
$ git clone <your-repo-url>
$ cd <your-repo>
$ git checkout -b <your-branch-name>
$ git status
$ ls
```

Then, you can run the program.

You have been provided with the compiled program: `mystery`.

```
$ cd src
$ ./mystery
```

When you are ready to begin debugging the program, you can execute GDB
on the binary by running:

```
$ gdb mystery
```

This drops you into the debugger, where you can set breakpoints, step through
the program, and examine memory.

You have been provided for the partial source code of the program. GDB requires
the source code in order to provide you with the C source and a mapping between
instructions and line numbers. 

If there was no source included in the lab, you would have to debug using the
dissassembled assembly language instructions. This is an important part about
how debuggers work -- they include information in the binary about _where_ in
the source code the machine code comes from. And this only works with programs
that include this information in the first place; production programs often
eschew debug information in favor of smaller or more efficient compiled
programs.

## Using GDB

Your textbook chapters
[3.0](https://diveintosystems.org/book/C3-C_debug/index.html),
[3.1](https://diveintosystems.org/book/C3-C_debug/gdb.html), and
[3.2](https://diveintosystems.org/book/C3-C_debug/gdb_commands.html) walk
through basic GDB commands. There is also
[gdb](https://www.cs.swarthmore.edu/~newhall/unixhelp/howto_gdb.php) resource
that has some basic HOWTO info on using GDB aimed at students.

In general, though, GDB behaves like any other debugger that you have used or
__will use__. It has the following basic functionality:

1. It can run your program. Use `run` if the program is not started, or
   `continue`/`cont` to resume execution after a breakpoint.
2. You can set break points--places where the program will stop executing and
   hand control over to the debugger. A break point is a way of saying "run to
   this place in the code (break point) and then stop so that I can examine the
   program state". Use `break <place>` to set a breakpoint (e.g., to break at
   the function `phase_1`, use `break phase_1`--this is a good strategy to
   follow for getting started on each phase!).
3. You can examine memory and print the state of variables. Use `print
   <expression>` to print the result of the expression. The expression can be a
   variable name or it can be an arithmetic expression involving variables.
   You can also use the `x` command to examine memory locations.
4. You can view the local variables in a function using the `info` command:
   `info locals`.
5. To view the call stack (the sequence of function calls that got the program
   to where it is right now), use `where`. Use `frame <number>` to go back to
   the stack frame of a calling function and view the state of the locals for
   that function.
6. Use `list` to view the source code around the line that will execute next.
7. Use `next` and `step` to run the next line. `next` will step over function
   calls (i.e., it will run the function call without taking you into it), and
   `step` will go into any function call.

I highly recommend reviewing chapter
[3.2](https://diveintosystems.org/book/C3-C_debug/gdb_commands.html), which
summarizes these commands and shows different uses of them. 

## Hand in

Create a file named `answers.md` in your branch by editing it with Vim. For each
of the phases, provide:

* the password that you used to successfully complete the phase and
* a description of what __other__ passwords could've been accepted. 

The second bullet point should sufficiently describe the possible passwords so
that I can see that you understand __what the phase does.__

If you worked with anyone else on this lab, please list them in that file and
include the relative contributions from each person.

## Tips

* Most of the source code is available to you so that you can use the debugger.
  It might be helpful to look at the `main` function and trace the behavior of
  the program from there.

  Here's where to locate files:

  ```
  |── README.md ------------ this file
  |── src
      |── Makefile --------- Makefile used to build the lab
      |── mystery ---------- compiled binary
      |── mystery.c -------- main function
      |── phases.c --------- main code for the phases
      |── phases.h --------- header file for the phases
      |── util.c ----------- utility code used by phases
      |── util.h ----------- header file for utility functions
  ```

  Note that this is the __incomplete source!__ There are other files that were
  used to compile the `mystery` binary.

* Take this one step at a time:
  
  * Set a breakpoint on the phase that you are currently working on.
  * Enter a random password and when you are dropped into the debugger, step
    through the phase function.
  * Print the value of variables the code is using to see their value.
  * Print comparison statements to see what the result of comparisons are.
  * Use the print command or the examine command to view locations in memory
    where passwords may be stored.
  * When you have a good idea of what the password is, or another guess that you
    would like to try, restart the program by running it again. 
  * Use `cont`/`continue` when you want to fast-forward through code to another
    breakpoint.

* If you get tired of the "fluff" in the lab, look at `main` for arguments that you
  can give the program.

* __Ask for help!__

* The purpose of this lab is to get you to use the debugger and to understand
  the semantics of C programs in a little greater detail. It is absolutely a
  good idea to do this lab concurrent with other work!

* There's a hidden phase 6. Can you find it? 👀
