#!/bin/bash

if [ "$1" == "clean" ]; then
  rm -f out.pcm
  rm -f out.speex
  rm -f test_speex
  exit 0
fi

gcc -g ../uni_speex.c test_speex.c uni_log.c -I./ -I../ -I../inc -L./ -lspeex_x86 -lm -o test_speex
if [ $? -eq 0 ]; then
  ./test_speex;
fi
