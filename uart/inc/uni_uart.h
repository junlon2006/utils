/**********************************************************************
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
 **********************************************************************
 *
 * Description : uni_uart.h
 * Author      : junlon2006@163.com
 * Date        : 2017.9.19
 *
 **********************************************************************/

#ifndef UART_INC_UNI_UART_H_
#define UART_INC_UNI_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <termios.h>

#define UNI_UART_DEVICE_NAME_MAX  (64)

typedef int (*UartFrameData)(char *buf, int len);

typedef struct {
  char    device[UNI_UART_DEVICE_NAME_MAX];
  speed_t speed;
} UartConfig;

int UartRegisterRecvFrameHandler(UartFrameData handler);
int UartUnregisterRecvFrameHandler();
int UartInitialize(UartConfig *config);
int UartFinalize();
int UartWrite(char *buf, int len);

#ifdef __cplusplus
}
#endif
#endif

