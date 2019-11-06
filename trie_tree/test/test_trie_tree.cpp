/*
 * All header file for c language MUST be include : in extern "C" { }
 */
extern "C" {
#include "../src/uni_trie_tree.c"
}

TrieTree *g_tree;
H2UNIT(trie_tree) {
  void setup() {
    g_tree = TrieTreeCreate();
  }
  void teardown() {
    TrieTreeDestroy(g_tree);
  }
};

H2CASE(trie_tree, "trie tree test") {
  int ret;
  /* 1. add test */
  ret = TrieTreeAdd(g_tree, "hello");
  H2EQ_MATH(0, ret);
  ret = TrieTreeAdd(g_tree, "helloworld");
  H2EQ_MATH(0, ret);
  ret = TrieTreeAdd(g_tree, "n1234567890");
  H2EQ_MATH(0, ret);
  ret = TrieTreeAdd(g_tree, "-_n1234567890");
  H2EQ_MATH(0, ret);
  ret = TrieTreeAdd(g_tree, NULL);
  H2EQ_MATH(-1, ret);
  ret = TrieTreeAdd(g_tree, "");
  H2EQ_MATH(-1, ret);
  ret = TrieTreeAdd(g_tree, "-_&n1234567890");
  H2EQ_MATH(-1, ret);
  /* 2. write test */
  ret = TrieTreeWrite(g_tree, "hello", 1);
  H2EQ_MATH(0, ret);
  ret = TrieTreeWrite(g_tree, "helloworld", 65535);
  H2EQ_MATH(0, ret);
  ret = TrieTreeWrite(g_tree, "n1234567890", -1);
  H2EQ_MATH(0, ret);
  ret = TrieTreeWrite(g_tree, "world", 0);
  H2EQ_MATH(-1, ret);
  ret = TrieTreeWrite(g_tree, NULL, 0);
  H2EQ_MATH(-1, ret);
  ret = TrieTreeWrite(g_tree, "", 0);
  H2EQ_MATH(-1, ret);
  /* 3. read test */
  int value;
  ret = TrieTreeRead(g_tree, "hello", &value);
  H2EQ_MATH(0, ret);
  H2EQ_MATH(1, value);
  ret = TrieTreeRead(g_tree, "helloworld", &value);
  H2EQ_MATH(0, ret);
  H2EQ_MATH(65535, value);
  ret = TrieTreeRead(g_tree, "n1234567890", &value);
  H2EQ_MATH(0, ret);
  H2EQ_MATH(-1, value);
  ret = TrieTreeRead(g_tree, "world", &value);
  H2EQ_MATH(-1, ret);
  ret = TrieTreeRead(g_tree, NULL, &value);
  H2EQ_MATH(-1, ret);
  ret = TrieTreeRead(g_tree, "", &value);
  H2EQ_MATH(-1, ret);
  /* 4. delete test */
  ret = TrieTreeDelete(g_tree, "hello");
  H2EQ_MATH(0, ret);
  ret = TrieTreeRead(g_tree, "hello", &value);
  H2EQ_MATH(-1, ret);
  ret = TrieTreeDelete(g_tree, "hello");
  H2EQ_MATH(-1, ret);
  ret = TrieTreeDelete(g_tree, "helloworld");
  H2EQ_MATH(0, ret);
  ret = TrieTreeRead(g_tree, "helloworld", &value);
  H2EQ_MATH(-1, ret);
  ret = TrieTreeDelete(g_tree, "n1234567890");
  H2EQ_MATH(0, ret);
  ret = TrieTreeRead(g_tree, "n1234567890", &value);
  H2EQ_MATH(-1, ret);
  ret = TrieTreeDelete(g_tree, "world");
  H2EQ_MATH(-1, ret);
  ret = TrieTreeDelete(g_tree, "");
  H2EQ_MATH(-1, ret);
  ret = TrieTreeDelete(g_tree, NULL);
  H2EQ_MATH(-1, ret);
}
