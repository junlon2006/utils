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
 * Description : uni_communication.h
 * Author      : junlon2006@163.com
 * Date        : 2017.9.19
 *
 **********************************************************************/

#ifndef UART_INC_UNI_COMMUNICATION_H_
#define UART_INC_UNI_COMMUNICATION_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short      CommCmd;
typedef unsigned short      CommPayloadLen;
typedef unsigned short      CommType;
typedef int                 (*CommWriteHandler)(char *buf, int len);

#define UNI_COMM_SYNC_VALUE (0xFF)
#define UNI_COMM_TYPE_BASE  (0)
#define UNI_DEMO            (UNI_COMM_TYPE_BASE + 1)
#define PACKED              __attribute__ ((packed))

typedef struct {
  CommCmd        cmd;
  CommPayloadLen payload_len;
  char           payload[0];
} PACKED CommPacket;

int        CommProtocolRegisterWriteHandler(CommWriteHandler handler);
int        CommProtocolPacketAssembleAndSend(CommType type, CommCmd cmd,
                                             char *payload,
                                             CommPayloadLen payload_len);
CommPacket *CommProtocolPacketDisassemble(char *buf, int len);
int         CommPacketFree(CommPacket *packet);

#ifdef __cplusplus
}
#endif
#endif
