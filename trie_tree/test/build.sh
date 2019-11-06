#!/bin/bash

if [ "$1" == "clean" ]; then
  rm -f test_trie_tree
  exit 0
fi

gcc -g test_trie_tree.c uni_log.c ../src/uni_trie_tree.c -I./ -I../inc -o test_trie_tree
./test_trie_tree
