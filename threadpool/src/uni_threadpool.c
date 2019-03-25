/**************************************************************************
 * Copyright (C) 2018-2019  Junlon2006
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
 * Description : uni_threadpool.c
 * Author      : junlon2006@163.com
 * Date        : 2019.03.24
 *
 **************************************************************************/
#include "uni_threadpool.h"

#include "uni_interruptable.h"
#include "list_head.h"
#include "uni_log.h"
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define uni_max(x,y)         ({ \
                                typeof(x) _x = (x); \
                                typeof(y) _y = (y); \
                                (void)(&_x == &_y); \
                                _x > _y ? _x : _y;})

#define THREADPOOL_TAG       "threadpool"

typedef struct {
  list_head        link;
  ThreadWorkerTask worker;
  void             *args;
} WorkerTask;

typedef struct {
  int             thread_cnt;
  int             is_running;
  pthread_t       *pids;
  list_head       *worker_lists;
  pthread_mutex_t *mutex;
  InterruptHandle *interrupthandle;
} ThreadPool;

typedef struct {
  ThreadPool *threadpool;
  int        thread_index;
} ThreadAttribute;

static WorkerTask *_get_worker(ThreadAttribute *attr) {
  pthread_mutex_t *mutex = &attr->threadpool->mutex[attr->thread_index];
  list_head *worker_list = &attr->threadpool->worker_lists[attr->thread_index];
  pthread_mutex_lock(mutex);
  WorkerTask *tsk = list_get_head_entry(worker_list, WorkerTask, link);
  if (NULL != tsk) {
    list_del(&tsk->link);
  }
  pthread_mutex_unlock(mutex);
  return tsk;
}

static void _destroy_worker_list(ThreadAttribute *attr) {
  int thread_index = attr->thread_index;
  list_head *head = &attr->threadpool->worker_lists[thread_index];
  WorkerTask *p, *t;
  list_for_each_entry_safe(p, t, head, WorkerTask, link) {
    list_del(&p->link);
    free(p);
  }
}

static void *_tpool_tsk(void *args) {
  ThreadAttribute *attr = (ThreadAttribute *)args;
  WorkerTask *tsk = NULL;
  while (attr->threadpool->is_running) {
    if (NULL != (tsk = _get_worker(attr))) {
      tsk->worker(tsk->args);
      free(tsk);
    }
    InterruptableSleep(attr->threadpool->interrupthandle[attr->thread_index],
                       1000 * 10);
  }
  _destroy_worker_list(attr);
  InterruptDestroy(attr->threadpool->interrupthandle[attr->thread_index]);
  pthread_mutex_destroy(&attr->threadpool->mutex[attr->thread_index]);
  free(attr);
  return NULL;
}

static void _try_destroy_all(ThreadPool *threadpool) {
  int i;
  threadpool->is_running = 0;
  for (i = 0; i < threadpool->thread_cnt; i++) {
    InterruptableBreak(threadpool->interrupthandle[i]);
  }
  for (i = 0; i < threadpool->thread_cnt; i++) {
    pthread_join(threadpool->pids[i], NULL);
  }
  if (NULL != threadpool->mutex) {
    free(threadpool->mutex);
  }
  if (NULL != threadpool->interrupthandle) {
    free(threadpool->interrupthandle);
  }
  if (NULL != threadpool->worker_lists) {
    free(threadpool->worker_lists);
  }
  if (NULL != threadpool->pids) {
    free(threadpool->pids);
  }
  free(threadpool);
}

static int _cpu_count(void) {
  int n = -1;
#if defined (_SC_NPROCESSORS_ONLN)
  n = (int) sysconf(_SC_NPROCESSORS_ONLN);
#elif defined (_SC_NPROC_ONLN)
  n = (int) sysconf(_SC_NPROC_ONLN);
#endif
  return n;
}

ThreadPoolHandle ThreadPoolCreate(int thread_cnt) {
  ThreadPool *threadpool = NULL;
  int i;
  if (thread_cnt == THREADPOOL_DEFAULT_THREAD_CNT) {
    thread_cnt = _cpu_count();
    LOGT(THREADPOOL_TAG, "cpu count=%d", thread_cnt);
    thread_cnt = uni_max(thread_cnt, 1);
    thread_cnt *= 2;
  }
  if (thread_cnt <= 0) {
    LOGE(THREADPOOL_TAG, "threadcnt[%d] invalid", thread_cnt);
    return NULL;
  }
  if (NULL == (threadpool = (ThreadPool *)malloc(sizeof(ThreadPool)))) {
    LOGE(THREADPOOL_TAG, "alloc memory failed");
    return NULL;
  }
  memset(threadpool, 0, sizeof(ThreadPool));
  if (NULL == (threadpool->pids = (pthread_t *)malloc(sizeof(pthread_t) *
                                                      thread_cnt))) {
    LOGE(THREADPOOL_TAG, "alloc memory failed");
    goto L_ERROR;
  }
  threadpool->interrupthandle = malloc(sizeof(InterruptHandle) * thread_cnt);
  if (NULL == threadpool->interrupthandle) {
    LOGE(THREADPOOL_TAG, "alloc memory failed");
    goto L_ERROR;
  }
  threadpool->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) *
                                                thread_cnt);
  if (NULL == threadpool->mutex) {
    LOGE(THREADPOOL_TAG, "alloc memory failed");
    goto L_ERROR;
  }
  for (i = 0; i < thread_cnt; i++) {
    pthread_mutex_init(&threadpool->mutex[i], 0);
  }
  threadpool->worker_lists = (list_head *)malloc(sizeof(list_head) * thread_cnt);
  if (NULL == threadpool->worker_lists) {
    LOGE(THREADPOOL_TAG, "alloc memory failed");
    goto L_ERROR;
  }
  for (i = 0; i < thread_cnt; i++) {
    list_init(&threadpool->worker_lists[i]);
  }
  threadpool->is_running = 1;
  for (i = 0; i < thread_cnt; i++) {
    ThreadAttribute *attr = (ThreadAttribute *)malloc(sizeof(ThreadAttribute));
    if (NULL == attr) {
      LOGE(THREADPOOL_TAG, "alloc memory failed");
      goto L_ERROR;
    }
    attr->thread_index = i;
    attr->threadpool = threadpool;
    attr->threadpool->interrupthandle[i] = InterruptCreate();
    if (NULL == attr->threadpool->interrupthandle[i]) {
      LOGE(THREADPOOL_TAG, "create interrupt sleep failed, current active "
           "thread_cnt=%d, recommend thread_cnt equal cpu count * 2 = %d",
           threadpool->thread_cnt, _cpu_count() * 2);
      free(attr);
      goto L_ERROR;
    }
    if (0 == pthread_create(&threadpool->pids[i], NULL, _tpool_tsk, attr)) {
      threadpool->thread_cnt++;
      continue;
    }
    InterruptDestroy(attr->threadpool->interrupthandle[i]);
    free(attr);
    LOGE(THREADPOOL_TAG, "pthread_create failed, errcode=%s", strerror(errno));
    goto L_ERROR;
  }
  srandom(time(NULL));
  LOGT(THREADPOOL_TAG, "active thread count=%d", threadpool->thread_cnt);
  return (ThreadPoolHandle)threadpool;
L_ERROR:
  _try_destroy_all(threadpool);
  return NULL;
}

void ThreadPoolDestroy(ThreadPoolHandle handle) {
  ThreadPool *threadpool = (ThreadPool *)handle;
  if (NULL == threadpool) {
    return;
  }
  _try_destroy_all(threadpool);
 }

static list_head *_get_idle_thread_worker_list(ThreadPool *threadpool,
                                               int *thread_index) {
  int i;
  for (i = 0; i < threadpool->thread_cnt; i++) {
    if (list_empty(&threadpool->worker_lists[i])) {
      *thread_index = i;
      return &threadpool->worker_lists[i];
    }
  }
  *thread_index = rand() % threadpool->thread_cnt;
  return &threadpool->worker_lists[*thread_index];
}

int ThreadPoolJoinWorker(ThreadPoolHandle handle,
                         ThreadWorkerTask worker, void *args) {
  ThreadPool *threadpool = (ThreadPool *)handle;
  list_head *worker_list = NULL;
  WorkerTask *tsk = NULL;
  int thread_index = 0;
  if (NULL == threadpool) {
    LOGE(THREADPOOL_TAG, "handle=%p, invalid", threadpool);
    return -1;
  }
  if (NULL == (tsk = (WorkerTask *)malloc(sizeof(WorkerTask)))) {
    LOGE(THREADPOOL_TAG, "alloc memory failed");
    return -1;
  }
  worker_list = _get_idle_thread_worker_list(threadpool, &thread_index);
  tsk->worker = worker;
  tsk->args = args;
  pthread_mutex_lock(&threadpool->mutex[thread_index]);
  list_add_tail(&tsk->link, worker_list);
  pthread_mutex_unlock(&threadpool->mutex[thread_index]);
  InterruptableBreak(threadpool->interrupthandle[thread_index]);
  return 0;
}
