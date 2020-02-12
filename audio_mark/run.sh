#!/bin/sh
cd code
gcc -g -o mark mark.c -L. -lengine_c
export LD_LIBRARY_PATH=.
ulimit -c unlimited
./mark