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
 * Description : uni_timer.c
 * Author      : junlon2006@163.com
 * Date        : 2019.03.17
 *
 **************************************************************************/
#include "uni_timer.h"

#include "list_head.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

static struct {
  list_head          timer_list;
  int                is_running;
  pthread_t          pid;
  unsigned long long id;
  pthread_mutex_t    mutex;
  pthread_cond_t     cond;
} g_timer = {0};

typedef struct {
  list_head           link;
  long long           period;
  long long           expire;
  unsigned long long  id;
  TimerExpireCallback fct;
  void                *arg;
} Timer;

static long long _relative_now(void) {
  long long now = 0;
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    now = (long long)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
  }
  return now;
}

static void _orderized_insert(Timer *rt) {
  list_head *p = NULL, *t = NULL;
  list_for_each_safe(p, t, &g_timer.timer_list) {
    Timer *c = list_entry(p, Timer, link);
    if (rt->expire < c->expire) {
      list_add_before(&rt->link, p);
      return;
    }
  }
  list_add_tail(&rt->link, &g_timer.timer_list);
}

static unsigned long long _get_id() {
  return ++g_timer.id;
}

static int _add_timer(Timer *rt, long long interval,
                             TimerType type, TimerExpireCallback fct,
                             unsigned long long id, void *arg) {
  rt->fct = fct;
  rt->arg = arg;
  rt->id = id;
  rt->expire = _relative_now() + interval;
  rt->period = (type == TIMER_TYPE_PERIODICAL ? interval : 0);
  _orderized_insert(rt);
  return 0;
}

static Timer* _is_timer_valid(unsigned long long id) {
  Timer *p = NULL;
  list_for_each_entry(p, &g_timer.timer_list, Timer, link) {
    if (p->id == id) {
      return p;
    }
  }
  return NULL;
}

static void _del_timer(Timer *rt) {
  list_del(&rt->link);
}

TimerHandle TimerStart(int interval_msec, TimerType type,
                       TimerExpireCallback fct, void *arg) {
  Timer *rt = (Timer *)malloc(sizeof(Timer));
  unsigned long long id = 0;
  if (interval_msec == 0) {
    interval_msec = 1;
  }
  memset(rt, 0, sizeof(Timer));
  list_init(&rt->link);
  pthread_mutex_lock(&g_timer.mutex);
  _add_timer(rt, interval_msec, type, fct, id, arg);
  pthread_cond_signal(&g_timer.cond);
  pthread_mutex_unlock(&g_timer.mutex);
  return (TimerHandle)id;
}

void TimerStop(TimerHandle handle) {
  unsigned long long id = (unsigned long long)handle;
  Timer *rt = NULL;
  pthread_mutex_lock(&g_timer.mutex);
  if (NULL != (rt = _is_timer_valid(id))) {
    _del_timer(rt);
    free(rt);
  }
  pthread_mutex_unlock(&g_timer.mutex);
}

static long long _timer_manage() {
  list_head *node;
  Timer *timer;
  int ret;
  long long now = _relative_now();
  do {
    pthread_mutex_lock(&g_timer.mutex);
    if (NULL == (node = list_get_head(&g_timer.timer_list))) {
      pthread_mutex_unlock(&g_timer.mutex);
      break;
    }
    timer = list_entry(node, Timer, link);
    if (now < timer->expire) {
      pthread_mutex_unlock(&g_timer.mutex);
      return timer->expire - now;
    }
    _del_timer(timer);
    ret = timer->fct(timer->arg);
    if (0 != timer->period && ret != TIMER_ERRNO_STOP_PERIODICAL) {
      _add_timer(timer, timer->period, TIMER_TYPE_PERIODICAL, timer->fct,
                 timer->id, timer->arg);
      pthread_mutex_unlock(&g_timer.mutex);
      continue;
    }
    pthread_mutex_unlock(&g_timer.mutex);
    free(timer);
  } while (node != NULL);
  return 1000;
}

static void _free_timer_nodes() {
  Timer *p, *t;
  list_for_each_entry_safe(p, t, &g_timer.timer_list, Timer, link) {
    list_del(&p->link);
    free(p);
  }
}

static void _free_all() {
  _free_timer_nodes();
  pthread_mutex_destroy(&g_timer.mutex);
  pthread_cond_destroy(&g_timer.cond);
}

static void *_timer_main(void *arg) {
  struct timespec ts;
  long long wait;
  long long timeout;
  while (g_timer.is_running) {
    wait = _timer_manage();
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timeout = (long long)ts.tv_sec * (long long)1000000000 +
              (long long)ts.tv_nsec;
    timeout += (wait * (long long)1000000);
    ts.tv_sec = (timeout / 1000000000);
    ts.tv_nsec = (timeout % 1000000000);
    pthread_mutex_lock(&g_timer.mutex);
    pthread_cond_timedwait(&g_timer.cond, &g_timer.mutex, &ts);
    pthread_mutex_unlock(&g_timer.mutex);
  }
  _free_all();
  return NULL;
}

int TimerInitialize(void) {
  list_init(&g_timer.timer_list);
  g_timer.is_running = 1;
  pthread_condattr_t attr;
  pthread_condattr_init(&attr);
  pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
  pthread_cond_init(&g_timer.cond, &attr);
  pthread_mutex_init(&g_timer.mutex, NULL);
  pthread_create(&g_timer.pid, NULL, _timer_main, NULL);
  return 0;
}

void TimerFinalize(void) {
  g_timer.is_running = 0;
  pthread_join(g_timer.pid, NULL);
}
