#include "uni_event_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
  char *context;
  int   msg_id;
  int   priority;
}EventInfo;

static void _event_handler(void *event) {
  EventInfo *event_info = (EventInfo*)event;
  printf("msg_id[%d], priority=%d, context=%s\n", event_info->msg_id, event_info->priority, event_info->context);
  //usleep(1000 * 10);
}

static void _event_free_handler(void *event) {
  static int cnt = 0;
  EventInfo *event_info = (EventInfo*)event;
  free(event_info->context);
  free(event_info);
  printf("free[%d]\n", ++cnt);
}

static void* _mult_thread_test_tsk(void *args) {
  EventListHandle handle = (EventListHandle)args;
  int count = 0;
  int test_loop = 10;
  while (test_loop--) {
    EventInfo *event = (EventInfo*)malloc(sizeof(EventInfo));
    event->context = (char*)malloc(1024);
    event->msg_id = count;
    if (test_loop % 3 == 0) {
      event->priority = EVENT_LIST_PRIORITY_HIGHEST;
    } else if (test_loop % 3 == 1) {
      event->priority = EVENT_LIST_PRIORITY_MEDIUM;
    } else {
      event->priority = EVENT_LIST_PRIORITY_LOWEST;
    }
    sprintf(event->context, "hello, this is an event context, count=%d", count++);
    EventListAdd(handle, event, event->priority);
    //usleep(1000 * 10);
  }
  EventListClear(handle);
  return NULL;
}

#define PTHREAD_CNT  (2)
int main(int argc, char const *argv[]) {
  pthread_t pid[PTHREAD_CNT];
  int i;
  EventListHandle handle = EventListCreate(_event_handler, _event_free_handler);
  usleep(1000 * 1000);
  for (i = 0; i < PTHREAD_CNT; i++) {
    pthread_create(&pid[i], NULL, _mult_thread_test_tsk, handle);
  }
  for (i = 0; i < PTHREAD_CNT; i++) {
    pthread_join(pid[i], NULL);
  }
  EventListDestroy(handle);
  return 0;
}
