#include "uni_ringbuf.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void* __read_tsk(void *args) {
  RingBufferHandle handle = (RingBufferHandle)args;
  char buf[64];
  while (1) {
    if (RingBufferGetDataSize(handle) > 0) {
      RingBufferRead(buf, 64, handle);
      printf("%s", buf);
    }
    usleep(1000 * 1);
  }
}

static void* __write_tsk(void *args) {
  RingBufferHandle handle = (RingBufferHandle)args;
  char buf[64];
  unsigned int index = 0;
  while (1) {
    while (RingBufferGetFreeSize(handle) >= 64) {
      snprintf(buf, sizeof(buf), "hello world: %u\n", ++index);
      RingBufferWrite(handle, buf, sizeof(buf));
    }
    usleep(1000 * 10);
  }
}

int main() {
  pthread_t pid[2];
  RingBufferHandle handle = RingBufferCreate(256);
  pthread_create(&pid[0], NULL, __read_tsk, handle);
  pthread_create(&pid[1], NULL, __write_tsk, handle);
  pthread_join(pid[0], NULL);
  pthread_join(pid[1], NULL);
  RingBufferDestroy(handle);
  return 0;
}
