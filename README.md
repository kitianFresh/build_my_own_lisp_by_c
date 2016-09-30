# build\_my\_own\_lisp\_by\_c

## A REPL -- read-evalute-print-loop program
 1. sovle the arrow key ^[[D or ^[[C by using libedit, readline and add\_history function;
 2. sudo apt-get install libedit-dev; cc -std=c99 -Wall prompt.c -ledit -o prompt;
 3. For the portability of the c program, use \#ifdef, \#else, and #endif preprocessor statements to tell compiler what code should it emit;

## [A Micro Parser Combinators By Daniel Holden](https://github.com/orangeduck/mpc)
 1. Add a Polish Notation parser;
 2. To use this lib, download the mpc.c and mpc.h in current directory, include "mpc.h" in your source file;
 3. use 'cc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing' to compile;

## Plumbing
&emsp;&emsp;In programming we call this *plumbing*. Roughly speaking this is following instructions to try to tie together a bunch of libraries or components, without fully understanding how they work internally.
