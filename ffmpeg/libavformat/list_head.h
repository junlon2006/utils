#ifndef UTILS_LIST_INC_LIST_HEAD_H_
#define UTILS_LIST_INC_LIST_HEAD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list_head {
  struct list_head *next, *prev;
} list_head;

#define LIST_HEAD_INIT(obj) { &(obj), &(obj) }

/* For the entry to be added , INIT is not required */
#define __LIST_ADD(entry,before,after) {list_head *new_= (entry), *prev = (before), *next = (after); (next)->prev = (new_); (new_)->next = (next); (new_)->prev = (prev); (prev)->next = (new_);}

/* void list_init(struct list_head *entry) */
#define list_init(entry) do {(entry)->next = (entry); (entry)->prev = (entry);} while(0)

/* void list_add(struct list_head *entry, struct list_head *base); */
#define list_add(entry,base) do { __LIST_ADD((entry),(base),(base)->next); } while(0)

/* void list_add_after(struct list_head *entry, struct list_head *base); */
#define list_add_after(entry,base) do { __LIST_ADD((entry),(base),(base)->next); } while(0)

/* void list_add_before(struct list_head *entry, struct list_head *base); */
#define list_add_before(entry,base) do { __LIST_ADD((entry),(base)->prev,(base)); } while(0)

/* void list_add_head(struct list_head *entry, struct list_head *head); */
#define list_add_head(entry,head) list_add_after(entry,head)
/* void list_add_tail(struct list_head *entry, struct list_head *head); */
#define list_add_tail(entry,head) list_add_before(entry,head)

/* void list_del(struct list_head *entry); */
#define list_del(entry) do { (entry)->prev->next = (entry)->next; (entry)->next->prev = (entry)->prev; (entry)->next = (entry)->prev = (entry);} while(0)

/*  int list_empty(struct list_head *head) */
#define list_empty(head) ((head)->next == (head))

int list_count(list_head *head);

/* struct list_head *
   list_get_head(struct list_head *head); */
#define list_get_head(head) (list_empty(head) ? (list_head*)NULL : (head)->next)
/* struct list_head *
   list_get_tail(struct list_head *head); */
#define list_get_tail(head) (list_empty(head) ? (list_head*)NULL : (head)->prev)

#define list_is_head(entry, head) ((entry)->prev == head)
#define list_is_tail(entry, head) ((entry)->next == head)

/* void list_enqueue(struct list_head *entry, struct list_head *head) */
#define list_enqueue list_add_tail

list_head* list_dequeue(list_head *head);

/* void list_push(struct list_head *entry, struct list_head *head) */
#define list_push list_add_tail

/* struct list_head* list_pop(struct list_head *head); */
list_head* list_pop(list_head *head);

#define list_entry(ptr, type, member) \
  ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_for_each(h, head) \
  for (h = (head)->next; h != (head); h = h->next)

#define list_for_each_safe(h, n, head) \
  for (h = (head)->next, n = h->next; h != (head);  h = n, n = h->next)

#define list_for_each_entry(p, head, type, member) \
  for (p = list_entry((head)->next, type, member); &p->member != (head); p = list_entry(p->member.next, type, member))

#define list_for_each_entry_safe(p, t, head, type, member) \
  for (p = list_entry((head)->next, type, member), t = list_entry(p->member.next, type, member); &p->member != (head); p = t, t = list_entry(t->member.next, type, member))

#define list_for_each_prev(h, head) \
  for( (h) = (head)->prev; (h) != (head); (h) = (h)->prev)

#define list_for_each_prev_safe(h, t, head) \
  for( (h) = (head)->prev, (t) = (h)->prev; (h) != (head); (h) = (t), (t) = (h)->prev)

#define list_get_head_entry(head, type, member) (list_empty(head) ? (type*)NULL : list_entry(((head)->next), type, member))
#define list_get_tail_entry(head, type, member) (list_empty(head) ? (type*)NULL : list_entry(((head)->prev), type, member))

#ifdef __cplusplus
}
#endif
#endif
