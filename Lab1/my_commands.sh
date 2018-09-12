#!/usr/bin/env bash
gcc -Wall -std=gnu99 -g -o echo_out.txt echo_arg.c
echo_out.txt csc209
gcc -Wall -std=gnu99 -g -o echo_stdin echo_stdin.c
echo_stdin < echo_stdin.c
gcc -Wall -std=gnu99 -g -o count count.c
count 210 | wc -c
gcc -Wall -std=gnu99 -g -o echo_stdin echo_stdin.c
ls -S | echo_stdin
