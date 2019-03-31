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
 * Description : uni_event.c
 * Author      : junlon2006@163.com
 * Date        : 2019.03.31
 *
 **************************************************************************/
#include "uni_event.h"

#include "uni_log.h"
#include "list_head.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define EVENT_TAG             "event"
#define EVENT_TYPE_HASH_CNT   (256)

typedef struct {
  list_head link;
  char      *type_str;
  int       type;
} EventType;

typedef struct {
  list_head       event_type_list[EVENT_TYPE_HASH_CNT];
  pthread_mutex_t mutex;
  int             id;
} EventStore;

static EventStore g_event_store;

int EventInit(void) {
  int i;
  for (i = 0; i < EVENT_TYPE_HASH_CNT; i++) {
    list_init(&g_event_store.event_type_list[i]);
  }
  pthread_mutex_init(&g_event_store.mutex, NULL);
  return 0;
}

static void _del_event_type(EventType *event_type) {
  list_del(&event_type->link);
  free(event_type->type_str);
  free(event_type);
}

static void _destroy_event_type_list() {
  EventType *p, *t;
  int i;
  for (i = 0; i < EVENT_TYPE_HASH_CNT; i++) {
    list_for_each_entry_safe(p, t, &g_event_store.event_type_list[i], EventType,
                             link) {
      _del_event_type(p);
    }
  }
}

void EventFinal(void) {
  _destroy_event_type_list();
  pthread_mutex_destroy(&g_event_store.mutex);
}

static bool _is_duplicated_event_type(const char *type_str) {
  int i;
  EventType *p;
  for (i = 0; i < EVENT_TYPE_HASH_CNT; i++) {
    list_for_each_entry(p, &g_event_store.event_type_list[i], EventType, link) {
      if (0 == strcmp(type_str, p->type_str)) {
        return true;
      }
    }
  }
  return false;
}

static int _get_event_type_hash_index(int type) {
  return type % EVENT_TYPE_HASH_CNT;
}

static int _add_new_event_type(const char *type_str) {
  int hash_index;
  EventType *event_type;
  if (NULL == (event_type = (EventType *)malloc(sizeof(EventType)))) {
    LOGE(EVENT_TAG, "alloc memory failed");
    return -1;
  }
  if (NULL == (event_type->type_str = (char *)malloc(strlen(type_str) + 1))) {
    free(event_type);
    LOGE(EVENT_TAG, "alloc memory failed");
    return -1;
  }
  event_type->type = g_event_store.id++;
  strcpy(event_type->type_str, type_str);
  hash_index = _get_event_type_hash_index(event_type->type);
  list_add_tail(&event_type->link, &g_event_store.event_type_list[hash_index]);
  LOGT(EVENT_TAG, "register new event[%d: %s]", event_type->type,
       event_type->type_str);
  return 0;
}

int EventTypeRegister(const char *type_str) {
  int err;
  pthread_mutex_lock(&g_event_store.mutex);
  if (_is_duplicated_event_type(type_str)) {
    LOGE(EVENT_TAG, "duplicated event type[%s], cannot register", type_str);
    exit(-1);
  }
  err = _add_new_event_type(type_str);
  pthread_mutex_unlock(&g_event_store.mutex);
  return err;
}

int EventTypeUnRegister(const char *type_str) {
  int i;
  EventType *p;
  for (i = 0; i < EVENT_TYPE_HASH_CNT; i++) {
    list_for_each_entry(p, &g_event_store.event_type_list[i], EventType, link) {
      if (0 == strcmp(type_str, p->type_str)) {
        _del_event_type(p);
        return 0;
      }
    }
  }
  return -1;
}

const char* EventGetStringByType(int type) {
  int idx = _get_event_type_hash_index(type);
  EventType *p;
  list_for_each_entry(p, &g_event_store.event_type_list[idx], EventType, link) {
    if (type == p->type) {
      return p->type_str;
    }
  }
  return NULL;
}

int EventGetTypeByString(const char *type_str) {
  EventType *p;
  int i;
  for (i = 0; i < EVENT_TYPE_HASH_CNT; i++) {
    list_for_each_entry(p, &g_event_store.event_type_list[i], EventType, link) {
      if (0 == strcmp(type_str, p->type_str)) {
        return p->type;
      }
    }
  }
  return EVENT_INVALID_TYPE;
}

void EventTypePrintAll(void) {
  EventType *p;
  int i;
  for (i = 0; i < EVENT_TYPE_HASH_CNT; i++) {
    list_for_each_entry(p, &g_event_store.event_type_list[i], EventType, link) {
      LOGT(EVENT_TAG, "[%d->%d:%s]", i, p->type, p->type_str);
    }
  }
}
