/**************************************************************************
 * Copyright (C) 2017-2017  junlon2006
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
 * Description : uni_monitor.c
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_monitor.h"

#include "list_head.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#define HASH_CNT_MAX  (32)
#define _MIN(a, b)    ((a) < (b) ? (a) : (b))
#define _MAX(a, b)    ((b) < (a) ? (a) : (b))

typedef struct {
  list_head          link;
  char               *key;
  unsigned long long cost_all;
  unsigned long long start_time;
  unsigned long long call_times;
  unsigned long long max;
  unsigned long long min;
} MonitorInfo;

static list_head g_monitor_list[HASH_CNT_MAX];

static inline int _str_2_hash_idx(const char *fmt) {
  int sum = 0;
  while (*fmt != '\0') sum += *fmt++;
  return sum % HASH_CNT_MAX;
}

static inline MonitorInfo* _found_key(const char *fmt) {
  MonitorInfo *p;
  list_head *head = &g_monitor_list[_str_2_hash_idx(fmt)];
  list_for_each_entry(p, head, MonitorInfo, link) {
    return p;
  }
  return NULL;
}

static inline long _now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + (tv.tv_usec / 1000);
}

static inline void _update_start_time(MonitorInfo *monitor) {
  monitor->start_time = _now();
  monitor->call_times++;
}

static inline MonitorInfo* _create_key(const char *fmt) {
  MonitorInfo *p = malloc(sizeof(MonitorInfo));
  memset(p, 0, sizeof(MonitorInfo));
  p->key = malloc(strlen(fmt) + 1);
  p->min = (unsigned long long)(-1);
  strcpy(p->key, fmt);
  list_add_tail(&p->link, &g_monitor_list[_str_2_hash_idx(fmt)]);
  return p;
}

static inline void _hash_str_key(char *dst, int len, const char *file,
                                 const char *function, int line) {
  snprintf(dst, len, "%s%s", file, function);
}

int MonitorFunctionBegin(const char *file, const char *function, int line) {
  char buf[256] = {0,};
  MonitorInfo *p;
  _hash_str_key(buf, sizeof(buf), file, function, line);
  if (NULL != (p = _found_key(buf))) {
    _update_start_time(p);
    return 0;
  }
  _update_start_time(_create_key(buf));
  return 0;
}

static inline void _update_end_time(MonitorInfo *monitor) {
  unsigned long long cost = _now() - monitor->start_time;
  monitor->cost_all += cost;
  monitor->min = _MIN(cost, monitor->min);
  monitor->max = _MAX(cost, monitor->max);
}

int MonitorFunctionEnd(const char *file, const char *function, int line) {
  char buf[256] = {0,};
  _hash_str_key(buf, sizeof(buf), file, function, line);
  _update_end_time(_found_key(buf));
  return 0;
}

int MonitorPrintStatus(const char *file, const char *function, int line) {
  MonitorInfo *p;
  int i;
  for (i = 0; i < HASH_CNT_MAX; i++) {
    list_for_each_entry(p, &g_monitor_list[i], MonitorInfo, link) {
      printf("key[%s]: call_times=%llu, avg=%llums, min=%llums, max=%llums\n",
        p->key, p->call_times, p->cost_all / p->call_times, p->min, p->max);
    }
  }
  return 0;
}

int MonitorInitialize() {
  int i;
  for (i = 0; i < HASH_CNT_MAX; i++) {
    list_init(&g_monitor_list[i]);
  }
  return 0;
}

int MonitorFinalize() {
  MonitorInfo *p, *t;
  int i;
  for (i = 0; i < HASH_CNT_MAX; i++) {
    list_for_each_entry_safe(p, t, &g_monitor_list[i], MonitorInfo, link) {
      list_del(&p->link);
      if (p->key) free(p->key);
      free(p);
    }
  }
  return 0;
}
