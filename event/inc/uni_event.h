/**************************************************************************
 * Copyright (C) 2018-2019 Junlon2006
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
 * Description : uni_event.h
 * Author      : junlon2006@163.com
 * Date        : 2019.03.31
 *
 **************************************************************************/
#ifndef EVENT_INC_UNI_EVENT_H_
#define EVENT_INC_UNI_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_INVALID_TYPE      (-1)

typedef struct {
  int  type;                                    /* event type */
  void *content;                                /* event content, need alloc */
  void (*content_free_handler)(void *content);  /* hook to free content */
} Event;

int EventInit(void);
void EventFinal(void);
int EventTypeRegister(const char *type_str);
int EventTypeUnRegister(const char *type_str);
int EventGetTypeByString(const char *type_str);
const char* EventGetStringByType(int type);
void EventTypePrintAll(void);

#ifdef __cplusplus
}   /* __cplusplus */
#endif
#endif  /* EVENT_INC_UNI_EVENT_H_ */
