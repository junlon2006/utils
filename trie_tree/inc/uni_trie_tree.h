/**************************************************************************
 * Copyright (C) 2017-2017  junlon2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : uni_trie_tree.h
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#ifndef TRIE_TREE_INC_UNI_TRIE_TREE_H_
#define TRIE_TREE_INC_UNI_TRIE_TREE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* 64 charactors: "a-z" "A-Z" "0-9" "_" "-" */
#define TRIE_MAX_CHILD 64

typedef struct TrieNode {
  struct TrieNode  *child[TRIE_MAX_CHILD];
  struct TrieNode  *parent;
  int              value;
  int              index;
  bool             active;
} TrieNode;

typedef TrieNode TrieTree;

int       TrieTreeAdd(TrieTree *tree, const char *str);
int       TrieTreeDelete(TrieTree *tree, const char *str);
int       TrieTreeWrite(TrieTree *tree, const char *str, int value);
int       TrieTreeRead(TrieTree *tree, const char *str, int *value);
int       TrieTreeExist(TrieTree *tree, const char *str);
void      TrieTreePrint(TrieTree *tree);
TrieTree* TrieTreeCreate(void);
void      TrieTreeDestroy(TrieTree *tree);

#ifdef __cplusplus
}
#endif
#endif  //  TRIE_TREE_INC_UNI_TRIE_TREE_H_
