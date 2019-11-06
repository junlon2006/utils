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
 * Description : uni_trie_tree.c
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/

#include "uni_trie_tree.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int _char2index(char c) {
  if (c >= 'a' && c <= 'z') {
    return (c - 'a');
  }
  if (c >= 'A' && c <= 'Z') {
    return (c - 'A' + 26);
  }
  if (c >= '0' && c <= '9') {
    return (c - '0' + 52);
  }
  if (c == '_') {
    return (TRIE_MAX_CHILD - 2);
  }
  if (c == '-') {
    return (TRIE_MAX_CHILD - 1);
  }
  return -1;
}

static char _index2char(int index) {
  if (index >= 0 && index < 26) {
    return ('a' + index);
  }
  if (index >= 26 && index < 52) {
    return ('A' + index - 26);
  }
  if (index >= 52 && index < 62) {
    return ('0' + index - 52);
  }
  if (index == TRIE_MAX_CHILD - 2) {
    return '_';
  }
  if (index == TRIE_MAX_CHILD - 1) {
    return '-';
  }
  return '@';
}

static int _check_str(const char *str) {
  int len;
  int i;
  if (NULL == str || 0 == (len = strlen(str))) {
    return -1;
  }
  for (i = 0; i < len; i++) {
    if (-1 == _char2index(str[i])) {
      return -1;
    }
  }
  return 0;
}

static int _child_num(TrieNode *node) {
  int i;
  int num = 0;
  if (NULL == node) {
    return 0;
  }
  for (i = 0; i < TRIE_MAX_CHILD; i++) {
    if (NULL != node->child[i]) {
      num++;
    }
  }
  return num;
}

static int _add_node_path(TrieNode *parent, const char *str) {
  TrieNode *node;
  int index;
  if ('\0' == *str) {
    parent->active = true;
    return 0;
  }
  index = _char2index(*str);
  node = parent->child[index];
  if (NULL == node) {
    node = (TrieNode *)malloc(sizeof(TrieNode));
    if (NULL == node) {
      return -1;
    }
    memset(node, 0, sizeof(TrieNode));
    node->parent = parent;
    parent->child[index] = node;
    node->index = index;
  }
  return _add_node_path(node, str + 1);
}

static int _remove_node_path(TrieNode *node) {
  TrieNode *parent;
  int index;
  if (NULL == node || node->active || _child_num(node)) {
    return 0;
  }
  parent = node->parent;
  index = node->index;
  free(node);
  parent->child[index] = NULL;
  return _remove_node_path(parent);
}

static TrieNode* _find_node(TrieTree *tree, const char *str) {
  int i = 0;
  TrieNode *p = tree;
  int len;
  int index;
  if (NULL == str || 0 == (len = strlen(str))) {
    return NULL;
  }
  for(;;) {
    index = _char2index(str[i]);
    if (NULL == p->child[index]) {
      break;
    }
    if (i < len - 1) {
      i++;
      p = p->child[index];
    } else {
      if (p->child[index]->active == true) {
        return p->child[index];
      } else {
        return NULL;
      }
    }
  }
  return NULL;
}

int TrieTreeAdd(TrieTree *tree, const char *str) {
  if (-1 == _check_str(str)) {
    return -1;
  }
  return _add_node_path(tree, str);
}

int TrieTreeDelete(TrieTree *tree, const char *str) {
  TrieNode *node;
  if (-1 == _check_str(str)) {
    return -1;
  }
  node = _find_node(tree, str);
  if (NULL == node) {
    return -1;
  }
  node->active = false;
  return _remove_node_path(node);
}

int TrieTreeWrite(TrieTree *tree, const char *str, int value) {
  TrieNode *node = _find_node(tree, str);
  if (NULL == node) {
    return -1;
  }
  node->value = value;
  return 0;
}

int TrieTreeRead(TrieTree *tree, const char *str, int *value) {
  TrieNode *node = _find_node(tree, str);
  if (NULL == node) {
    return -1;
  }
  *value = node->value;
  return 0;
}

static void _print_node(TrieNode *node, char *str, int depth) {
  int i;
  if (NULL == node) {
    return;
  }
  *(str + depth) = _index2char(node->index);
  if (true == node->active) {
    *(str + depth + 1) = '\0';
    printf("%s: %d\n", str + 1, node->value);
  }
  for (i = 0; i < TRIE_MAX_CHILD; i++) {
    _print_node(node->child[i], str, depth + 1);
  }
}

int TrieTreeExist(TrieTree *tree, const char *str) {
  if (NULL == _find_node(tree, str)) {
    return false;
  }
  return true;
}

void TrieTreePrint(TrieTree *tree) {
  char str[640];
  _print_node(tree, str, 0);
}

TrieTree* TrieTreeCreate(void) {
  TrieTree *tree = (TrieTree *)malloc(sizeof(TrieTree));
  memset(tree, 0, sizeof(TrieTree));
  tree->parent = NULL;
  tree->index = -1;
  return tree;
}

void TrieTreeDestroy(TrieTree *tree) {
  int i;
  for(i = 0; i < TRIE_MAX_CHILD; i++) {
    if(NULL != tree->child[i]) {
      TrieTreeDestroy(tree->child[i]);
    }
  }
  free(tree);
}
