#include "list_head.h"
#include <stdlib.h>

list_head* list_dequeue(list_head *head) {
  list_head *t;
  if (list_empty(head)) {
    return (list_head*) NULL;
  }
  t = head->next;
  list_del(t);
  return t;
}

list_head* list_pop(list_head *head) {
  list_head *t;
  if (list_empty(head)) {
    return (list_head*) NULL;
  }
  t = head->prev;
  list_del(t);
  return t;
}

int list_count(list_head *head) {
  int c = 0;
  list_head *t;
  list_for_each(t, head) {
    c++;
  }
  return c;
}
