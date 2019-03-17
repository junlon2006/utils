#include "uni_interruptable.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#define SLEEP_MSEC           (1000 * 1000)

static void* __block_task(void *args) {
  InterruptHandle handle = (InterruptHandle)args;
  printf("%s%d: sleep start...\n", __FUNCTION__, __LINE__);
  InterruptableSleep(handle, SLEEP_MSEC);
  printf("%s%d: sleep end...\n", __FUNCTION__, __LINE__);
}

int main() {
  pthread_t pid;
  InterruptHandle handle = InterruptCreate();
  pthread_create(&pid, NULL, __block_task, handle);
  pthread_detach(pid);
  sleep(1);
  InterruptableBreak(handle);
  sleep(3);
  InterruptDestroy(handle);
  return 0;
}
