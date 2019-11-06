#!/bin/bash

echo $1
if [ "$1" == "clean" ]; then
  rm -f out.pcm
  rm -f out.adpcm
  rm -f test_adpcm
  exit 0
fi

gcc -g ../uni_adpcm.c test_adpcm.c -I./ -I../ -o test_adpcm
if [ $? -eq 0 ]; then
  ./test_adpcm;
fi
