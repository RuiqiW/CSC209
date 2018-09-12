#!/usr/bin/env bash
gcc -Wall -std=gnu99 -g -o hello hello.c
hello
gcc -Wall -std=gnu99 -g -o echo_stdin echo_stdin.c
echo_stdin
gcc -Wall -std=gnu99 -g -o count count.c
count 10
gcc -Wall -std=gnu99 -g -o echo_arg echo_arg.c
echo_arg world


