/**************************************************************************
 * Copyright (C) 2017-2017  Junlon2006
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
 * Description : uni_uart.c
 * Author      : junlon2006@163.com
 * Date        : 2019.12.28
 *
 **************************************************************************/

#include "uni_uart.h"
#include "uni_iot.h"
#include "uni_log.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>

#define UART_TAG  "uart"

static RecvUartDataHandler _on_recv_uart_data = NULL;

static void _register_uart_recv_hook(RecvUartDataHandler handler) {
  _on_recv_uart_data = handler;
}

static void _uart_device_init() {
}

int UartWrite(char *buf, int len) {
  return len;
}

int UartInitialize(RecvUartDataHandler handler) {
  _uart_device_init();
  _register_uart_recv_hook(handler);
  return 0;
}

void UartFinalize() {
  _recv_uart_data_handler(NULL);
  return;
}
