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
 * Description : uni_communication.c
 * Author      : junlon2006@163.com
 * Date        : 2017.9.19
 *
 **********************************************************************/

#include "uni_communication.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

/*--------------------------------------------------------------------------*/
/*               layout of UART communication app protocol                  */
/*--------------------------------------------------------------------------*/
/*---1byte----|---2byte---|---2byte---|---2byte---|---2byte---|---N byte----*/
/*    帧头    |   类型    |    cmd    | checksum  |payload len|   payload   */
/*--------------------------------------------------------------------------*/

typedef unsigned short CommChecksum;
typedef unsigned char  CommSync;

typedef struct {
  CommSync       sync;
  CommType       type;
  CommCmd        cmd;
  CommChecksum   checksum;
  CommPayloadLen payload_len;
  char           payload[0];
} PACKED CommProtocolPacket;

typedef enum {
  E_UNI_COMM_ALLOC_FAILED = -10001,
  E_UNI_COMM_BUFFER_PTR_NULL,
} CommProtocolErrorCode;

static CommWriteHandler _on_write = NULL;

int CommProtocolRegisterWriteHandler(CommWriteHandler handler) {
  _on_write = handler;
  return 0;
}

char* uni_comm_error_code_2_string(CommProtocolErrorCode code) {
  switch (code) {
  case E_UNI_COMM_ALLOC_FAILED:
    return "E_UNI_COMM_ALLOC_FAILED";
  case E_UNI_COMM_BUFFER_PTR_NULL:
    return "E_UNI_COMM_BUFFER_PTR_NULL";
  default:
    return "null";
  }
}

static int _uni_comm_protocol_sync_set(CommProtocolPacket *packet) {
  packet->sync = UNI_COMM_SYNC_VALUE;
  return 0;
}

CommSync uni_comm_protocol_sync_get(CommProtocolPacket *packet) {
  return packet->sync;
}

static int _uni_comm_protocol_product_type_set(CommProtocolPacket *packet,
                                               CommType type) {
  packet->type = type;
  return 0;
}

CommType uni_comm_protocol_product_type_get(CommProtocolPacket *packet,
                                            CommType type) {
  return packet->type;
}

static int _uni_comm_protocol_cmd_set(CommProtocolPacket *packet,
                                      CommCmd cmd) {
  packet->cmd = cmd;
  return 0;
}

CommCmd uni_comm_protocol_cmd_get(CommProtocolPacket *packet) {
  return packet->cmd;
}

static int _uni_comm_protocol_payload_len_set(CommProtocolPacket *packet,
                                              CommPayloadLen payload_len) {
  packet->payload_len = payload_len;
  return 0;
}

static CommPayloadLen _uni_comm_protocol_payload_len_get(CommProtocolPacket *packet) {
  return packet->payload_len;
}

static int _uni_comm_protocol_payload_set(CommProtocolPacket *packet,
                                          char *buf,
                                          CommPayloadLen len) {
  if (NULL != buf && 0 < len) {
    memcpy(packet->payload, buf, len);
  }
  return 0;
}

static char* _uni_comm_protocol_payload_get(CommProtocolPacket *packet) {
  return packet->payload;
}

static CommPayloadLen _uni_comm_protocol_packet_len_get(CommProtocolPacket *packet) {
  return sizeof(CommProtocolPacket) + packet->payload_len;
}

static int _uni_comm_protocol_checksum_calc(CommProtocolPacket *packet) {
  int packet_len = _uni_comm_protocol_packet_len_get(packet);
  int i;
  unsigned short checksum = 0;
  packet->checksum = 0; /* make sure the checksum be zero before calculate */
  for (i = 0; i < packet_len; ++i) {
    checksum += ((char *)packet)[i]; /* could parse to u64 to optimize, but not necessary */
  }
  packet->checksum = checksum;
  return 0;
}

CommChecksum uni_comm_protocol_checksum_get(CommProtocolPacket *packet) {
  return packet->checksum;
}

static int _uni_comm_protocol_checksum_valid(CommProtocolPacket *packet) {
  CommChecksum checksum = packet->checksum; /* get the checksum from packet */
  _uni_comm_protocol_checksum_calc(packet); /* calc checksum again */
  return checksum == packet->checksum; /* check whether checksum valid or not */
}

#define UNI_COMM_PACKET_ALLOC(payload_len) \
  ((CommProtocolPacket *)(malloc(sizeof(CommProtocolPacket) + payload_len)))
#define UNI_COMM_PACKET_FREE(packet) (free(packet))
int CommProtocolPacketAssembleAndSend(CommType type, CommCmd cmd,
                                      char *payload, CommPayloadLen payload_len) {
  CommProtocolPacket *packet = UNI_COMM_PACKET_ALLOC(payload_len);
  if (NULL == packet) {
    return E_UNI_COMM_ALLOC_FAILED;
  }
  _uni_comm_protocol_sync_set(packet);
  _uni_comm_protocol_product_type_set(packet, type);
  _uni_comm_protocol_cmd_set(packet, cmd);
  _uni_comm_protocol_payload_set(packet, payload, payload_len);
  _uni_comm_protocol_payload_len_set(packet, payload_len);
  _uni_comm_protocol_checksum_calc(packet);
  if (_on_write) {
    _on_write((char *)packet, (int)_uni_comm_protocol_packet_len_get(packet));
  }
  UNI_COMM_PACKET_FREE(packet);
  return 0;
}

int CommPacketFree(CommPacket *packet) {
  if (NULL != packet) {
    free(packet);
  }
  return 0;
}

CommPacket* CommProtocolPacketDisassemble(char *buf, int len) {
  CommPacket *packet = NULL;
  CommProtocolPacket *protocol_packet = (CommProtocolPacket *)buf;
  int alloc_length = 0;
  if (NULL == protocol_packet) {
    return NULL;
  }
  alloc_length = sizeof(CommPacket);
  alloc_length += _uni_comm_protocol_payload_len_get(protocol_packet);
  packet = malloc(alloc_length);
  if (NULL == packet) {
    return NULL;
  }
  if (!_uni_comm_protocol_checksum_valid(protocol_packet)) {
    return NULL;
  }
  packet->cmd = protocol_packet->cmd;
  packet->payload_len = _uni_comm_protocol_payload_len_get(protocol_packet);
  memcpy(packet->payload, _uni_comm_protocol_payload_get(protocol_packet),
         _uni_comm_protocol_payload_len_get(protocol_packet));
  return packet;
}
