#include "uni_communication.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

/*--------------------------------------------------------------------------*/
/*            layout of uni_sound communication app protocol                */
/*--------------------------------------------------------------------------*/
/*---1byte----|---2byte---|---2byte---|---2byte---|---2byte---|---N byte----*/
/*    帧头    |   类型    |    cmd    | checksum  |payload len|   payload   */
/*--------------------------------------------------------------------------*/

typedef unsigned short UniCommChecksum;
typedef unsigned char  UniCommSync;

typedef struct {
  UniCommSync       sync;
  UniCommType       type;
  UniCommCmd        cmd;
  UniCommChecksum   checksum;
  UniCommPayloadLen payload_len;
  char              payload[0];
} PACKED UniCommProtocolPacket;

typedef enum {
  E_UNI_COMM_ALLOC_FAILED = -10001,
  E_UNI_COMM_BUFFER_PTR_NULL,
} UniCommProtocolErrorCode;

static UniCommWriteHandler _on_write = NULL;

int UniCommProtocolRegisterWriteHandler(UniCommWriteHandler handler) {
  _on_write = handler;
  return 0;
}

char* uni_comm_error_code_2_string(UniCommProtocolErrorCode code) {
  switch (code) {
  case E_UNI_COMM_ALLOC_FAILED:
    return "E_UNI_COMM_ALLOC_FAILED";
  case E_UNI_COMM_BUFFER_PTR_NULL:
    return "E_UNI_COMM_BUFFER_PTR_NULL";
  default:
    return "null";
  }
}

static int _uni_comm_protocol_sync_set(UniCommProtocolPacket *packet) {
  packet->sync = UNI_COMM_SYNC_VALUE;
  return 0;
}

UniCommSync uni_comm_protocol_sync_get(UniCommProtocolPacket *packet) {
  return packet->sync;
}

static int _uni_comm_protocol_product_type_set(UniCommProtocolPacket *packet,
                                               UniCommType type) {
  packet->type = type;
  return 0;
}

UniCommType uni_comm_protocol_product_type_get(UniCommProtocolPacket *packet,
                                               UniCommType type) {
  return packet->type;
}

static int _uni_comm_protocol_cmd_set(UniCommProtocolPacket *packet,
                                      UniCommCmd cmd) {
  packet->cmd = cmd;
  return 0;
}

UniCommCmd uni_comm_protocol_cmd_get(UniCommProtocolPacket *packet) {
  return packet->cmd;
}

static int _uni_comm_protocol_payload_len_set(UniCommProtocolPacket *packet,
                                              UniCommPayloadLen payload_len) {
  packet->payload_len = payload_len;
  return 0;
}

static UniCommPayloadLen _uni_comm_protocol_payload_len_get(UniCommProtocolPacket *packet) {
  return packet->payload_len;
}

static int _uni_comm_protocol_payload_set(UniCommProtocolPacket *packet,
                                          char *buf,
                                          UniCommPayloadLen len) {
  if (NULL != buf && 0 < len) {
    memcpy(packet->payload, buf, len);
  }
  return 0;
}

static char* _uni_comm_protocol_payload_get(UniCommProtocolPacket *packet) {
  return packet->payload;
}

static UniCommPayloadLen _uni_comm_protocol_packet_len_get(UniCommProtocolPacket *packet) {
  return sizeof(UniCommProtocolPacket) + packet->payload_len;
}

static int _uni_comm_protocol_checksum_calc(UniCommProtocolPacket *packet) {
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

UniCommChecksum uni_comm_protocol_checksum_get(UniCommProtocolPacket *packet) {
  return packet->checksum;
}

static int _uni_comm_protocol_checksum_valid(UniCommProtocolPacket *packet) {
  UniCommChecksum checksum = packet->checksum; /* get the checksum from packet */
  _uni_comm_protocol_checksum_calc(packet); /* calc checksum again */
  return checksum == packet->checksum; /* check whether checksum valid or not */
}

#define UNI_COMM_PACKET_ALLOC(payload_len) \
  ((UniCommProtocolPacket *)(malloc(sizeof(UniCommProtocolPacket) + payload_len)))
#define UNI_COMM_PACKET_FREE(packet) (free(packet))
int UniCommProtocolPacketAssembleAndSend(UniCommType type, UniCommCmd cmd,
                                         char *payload,
                                         UniCommPayloadLen payload_len) {
  UniCommProtocolPacket *packet = UNI_COMM_PACKET_ALLOC(payload_len);
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

int UniCommPacketFree(UniCommPacket *packet) {
  if (NULL != packet) {
    free(packet);
  }
  return 0;
}

UniCommPacket* UniCommProtocolPacketDisassemble(char *buf, int len) {
  UniCommPacket *packet = NULL;
  UniCommProtocolPacket *protocol_packet = (UniCommProtocolPacket *)buf;
  int alloc_length = 0;
  if (NULL == protocol_packet) {
    return NULL;
  }
  alloc_length = sizeof(UniCommPacket);
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
