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
 * Description : uni_blackboard.c
 * Author      : junlon2006@163.com
 * Date        : 2019.12.21
 *
 **************************************************************************/

#include "uni_blackbord.h"
#include <pthread.h>
#include <string.h>

typedef struct {
  VALUE            table[BB_KEY_CNT];
  FreeValueHandler free_handler[BB_KEY_CNT];
  pthread_mutex_t  mutex;
} Blackboard;

static Blackboard g_blackboard;

int BlackbordInit() {
  memset(&g_blackboard, 0, sizeof(g_blackboard));
  pthread_mutex_init(&g_blackboard.mutex, NULL);
  return 0;
}

void BlackboardFinal() {
  int i;
  for (i = 0; i < BB_KEY_CNT; i++) {
    if (NULL != g_blackboard.free_handler[i]) {
      g_blackboard.free_handler[i](g_blackboard.table[i]);
    }
  }

  pthread_mutex_destroy(&g_blackboard.mutex);
}

VALUE BlackboardRead(BlackboardKey key) {
  VALUE value;
  if (key <= BB_KEY_INVALID || key >= BB_KEY_CNT) {
    return NULL;
  }

  pthread_mutex_lock(&g_blackboard.mutex);
  value = g_blackboard.table[key];
  pthread_mutex_unlock(&g_blackboard.mutex);
  return value;
}

int BlackboardWrite(BlackboardKey key, VALUE value, FreeValueHandler free_handler) {
  if (key <= BB_KEY_INVALID || key >= BB_KEY_CNT) {
    return -1;
  }

  pthread_mutex_lock(&g_blackboard.mutex);
  if (g_blackboard.free_handler[key] != free_handler) {
    if (g_blackboard.free_handler[key] != NULL) {
      g_blackboard.free_handler[key](g_blackboard.table[key]);
    }
    g_blackboard.free_handler[key] = free_handler;
  }
  g_blackboard.table[key] = value;
  pthread_mutex_unlock(&g_blackboard.mutex);

  return 0;
}