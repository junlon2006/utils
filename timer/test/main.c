#include "uni_timer.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static int count = 0;
static int _timer_handler(void *arg) {
  count++;
  printf("hello, I`m a periodical timer count=[%d].\n", ++count);
  return 0;
}

#define HANDLE_CNT  10
static void *_start_timer_tsk(void *arg) {
  int i;
  TimerHandle handle[HANDLE_CNT];
  for(i = 0; i < HANDLE_CNT; i++) {
    handle[i] = TimerStart(50, TIMER_TYPE_PERIODICAL, _timer_handler, NULL);
  }
  sleep(5);
  for (i = 0; i < HANDLE_CNT; i++) {
    TimerStop(handle[i]);
  }
}

#define THREAD_CNT  10
int main(int argc, char const *argv[]) {
  pthread_t pid[THREAD_CNT];
  int i;
  /* step 1. init timer module*/
  TimerInitialize();
  /* step 2. muti-thread start timers */
  for (i = 0; i < THREAD_CNT; i++) {
    pthread_create(&pid[i], NULL, _start_timer_tsk, NULL);
  }
  for (i = 0; i < THREAD_CNT; i++) {
    pthread_join(pid[i], NULL);
  }
  /* step 4. module finalize */
  TimerFinalize();
  printf("count=%d\n", count);
  return 0;
}
