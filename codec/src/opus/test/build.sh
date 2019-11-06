#!/bin/bash

echo $1
if [ "$1" == "clean" ]; then
  rm -f out.pcm
  rm -f out.opus
  rm -f test_opus
  exit 0
fi

gcc -g ../uni_opus.c test_opus.c -I../ -I../inc -L./ -lopus_x86 -lm -o test_opus
if [ $? -eq 0 ]; then
  ./test_opus;
fi
