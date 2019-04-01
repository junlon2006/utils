#include "uni_log.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define MAIN_TAG "main_tag"
void debug() {
  static int count = 0;
  LOGD("debug_tag", "hello world. this is debug log, count=%d", ++count);
}

void info() {
  static int count = 0;
  LOGT("info_tag", "hello world. this is info log, count=%d", ++count);
}

void warn() {
  static int count = 0;
  LOGW("warn_tag", "hello world. this is warn log, count=%d", ++count);
}

void error() {
  static int count = 0;
  static pthread_mutex_t mutex;
  pthread_mutex_lock(&mutex);
  count++;
  pthread_mutex_unlock(&mutex);
  LOGE("error_tag", "hello world. this is error log, count=%d", count);
}

static void* thread_func(void *arg) {
  int count = 10000;
  while (1) {
    while (count--) {
      debug();
      info();
      warn();
      error();
      usleep(10000);
    }
    count = 10000;
    sleep(3);
  }
  return arg;
}

static void* __tsk(void *args) {
  while (1) sleep(1);
  return NULL;
}

static void __test() {
  pthread_t pid;
  int count = 1;
  while (1) {
    pthread_create(&pid, NULL, __tsk, NULL);
    count ++;
    LOGE(MAIN_TAG, "count=%d", count);
    usleep(1000 * 1);
  }
}

#define PTHREAD_CNT 20
int main (int argc, char const *argv[]) {
  int count = 0;
  pthread_t pid[PTHREAD_CNT];

  /*step 1. init config*/
  LogConfig config = {1, 1, 1, 1, 0, N_LOG_ALL};
  LogInitialize(config);

  LogLevelSet(N_LOG_ALL);

  __test();

  /*step 2. use multi-thread test*/
  for (int i = 0; i < PTHREAD_CNT; i++) {
    pthread_create(&pid[i], NULL, thread_func, NULL);
  }
  for (int i = 0; i < PTHREAD_CNT; i++) {
    pthread_join(pid[i], NULL);
  }
  /*step 3. finalize*/
  LogFinalize();
  return 0;
}

