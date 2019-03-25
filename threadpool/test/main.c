#include "uni_threadpool.h"
#include "uni_log.h"
#include <stdio.h>
#include <unistd.h>

#define MAIN_TAG  "main"

static void *__test_tsk(void *args) {
  int count = *(int *)args;
  LOGT(MAIN_TAG, "this is a callback[%d]", count);
  return NULL;
}

int main() {
  ThreadPoolHandle handle = ThreadPoolCreate(THREADPOOL_DEFAULT_THREAD_CNT);
  static int count = 0;
  while (1) {
    //if (count++ == 100) break;
    count++;
    ThreadPoolJoinWorker(handle, __test_tsk, &count);
    usleep(1000 * 100);
  }
  ThreadPoolDestroy(handle);
  return 0;
}
