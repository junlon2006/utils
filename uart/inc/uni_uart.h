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
 * Date        : 2019.11.28
 *
 **********************************************************************/

#ifndef UART_INC_UNI_UART_H_
#define UART_INC_UNI_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*RecvUartDataHandler)(char *buf, int len);

/**
 * @brief uart init
 * @param handler handle uart receive data hook
 * @return 0 means success, -1 means failed
 */
int UartInitialize(RecvUartDataHandler handler);

/**
 * @brief uart finalize
 * @param void
 * @return void
 */
void UartFinalize(void);

/**
 * @brief write data by UART, multi-thread unsafe, please write in sync mode
 * @param buf the data buffer to write
 * @param len the data length
 * @return the actual write length by UART
 */
int UartWrite(char *buf, int len);

#ifdef __cplusplus
}
#endif
#endif

