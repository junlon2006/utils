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
 * Description : uni_communication.c
 * Author      : junlon2006@163.com
 * Date        : 2019.12.27
 *
 **************************************************************************/
#include "uni_communication.h"

#include "uni_log.h"
#include "uni_crc16.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define UART_COMM_TAG                 "uart_comm"
#define UNI_COMM_SYNC_VALUE           (0xFF)

#define DEFAULT_PROTOCOL_BUF_SIZE     (16)
#define PROTOCOL_BUF_GC_TRIGGER_SIZE  (256)
#define PROTOCOL_BUF_SUPPORT_MAX_SIZE (8192)

#define WAIT_ACK_TIMEOUT_MSEC_MIN     (50)
#define WAIT_ACK_TIMEOUT_MSEC_MAX     (2000)

#define uni_min(x, y)                 (x < y ? x : y)
#define uni_max(x, y)                 (x > y ? x : y)
#define uni_malloc                    malloc
#define uni_free                      free
#define false                         0
#define true                          1

/*----------------------------------------------------------------------------*/
/*            layout of uni_sound communication app protocol                  */
/*----------------------------------------------------------------------------*/
/*-1byte-|-1byte-|---2byte---|---2byte---|---2byte---|---2byte---|---N byte---*/
/*  SYNC |seq-num| customer  |  command  | checksum  |payload len|   payload  */
/*----------------------------------------------------------------------------*/

/*--------------------------------ack frame-----------------------------------*/
/*  0xFF | seqNum|    0x0    |   0x0     |   0xFF    |    0x0    |    NULL    */
/*----------------------------------------------------------------------------*/

typedef unsigned short CommChecksum;
typedef unsigned char  CommSync;
typedef unsigned char  CommSequence;

typedef enum {
  LAYOUT_SYNC_IDX             = 0,
  LAYOUT_PAYLOAD_LEN_LOW_IDX  = 8,
  LAYOUT_PAYLOAD_LEN_HIGH_IDX = 9,
} CommLayoutIndex;

typedef struct {
  CommSync       sync;      /* must be UNI_COMM_SYNC_VALUE */
  CommSequence   sequence;  /* sequence number */
  CommType       type;      /* customer type */
  CommCmd        cmd;       /* command type, such as power on, power off etc */
  CommChecksum   checksum;      /* checksum of packet, use crc16 */
  CommPayloadLen payload_len;   /* the length of payload */
  char           payload[0];    /* the payload */
} PACKED CommProtocolPacket;

typedef enum {
  E_UNI_COMM_ALLOC_FAILED = -10001,
  E_UNI_COMM_BUFFER_PTR_NULL,
  E_UNI_COMM_PAYLOAD_TOO_LONG,
  E_UNI_COMM_PAYLOAD_ACK_TIMEOUT,
} CommProtocolErrorCode;

typedef struct {
  CommWriteHandler      on_write;
  RecvCommPacketHandler on_recv_frame;
  pthread_mutex_t       mutex;
  uni_bool              acked;
  CommSequence          sequence;
} CommProtocolBusiness;

static CommProtocolBusiness g_comm_protocol_business;

static void _register_write_handler(CommWriteHandler handler) {
  g_comm_protocol_business.on_write = handler;
}

static void _unregister_write_handler() {
  g_comm_protocol_business.on_write = NULL;
}

static int _sync_set(CommProtocolPacket *packet) {
  packet->sync = UNI_COMM_SYNC_VALUE;
  return 0;
}

static int _sequence_set(CommProtocolPacket *packet) {
  packet->sequence = g_comm_protocol_business.sequence++;
  return 0;
}

static CommSequence _current_sequence_get() {
  return g_comm_protocol_business.sequence - 1;
}

static int _product_type_set(CommProtocolPacket *packet, CommType type) {
  packet->type = type;
  return 0;
}

CommType uni_comm_protocol_product_type_get(CommProtocolPacket *packet,
                                            CommType type) {
  return packet->type;
}

static int _cmd_set(CommProtocolPacket *packet, CommCmd cmd) {
  packet->cmd = cmd;
  return 0;
}

static int _payload_len_set(CommProtocolPacket *packet,
                            CommPayloadLen payload_len) {
  packet->payload_len = payload_len;
  return 0;
}

static CommPayloadLen _payload_len_get(CommProtocolPacket *packet) {
  return packet->payload_len;
}

static int _payload_set(CommProtocolPacket *packet,
                        char *buf, CommPayloadLen len) {
  if (NULL != buf && 0 < len) {
    memcpy(packet->payload, buf, len);
  }
  return 0;
}

static char* _payload_get(CommProtocolPacket *packet) {
  return packet->payload;
}

static CommPayloadLen _packet_len_get(CommProtocolPacket *packet) {
  return sizeof(CommProtocolPacket) + packet->payload_len;
}

static int _checksum_calc(CommProtocolPacket *packet) {
  packet->checksum = 0; /* make sure the checksum be zero before calculate */
  packet->checksum = crc16((const char*)packet, _packet_len_get(packet));
  return 0;
}

static int _checksum_valid(CommProtocolPacket *packet) {
  CommChecksum checksum = packet->checksum; /* get the checksum from packet */
  _checksum_calc(packet); /* calc checksum again */
  return checksum == packet->checksum; /* check whether checksum valid or not */
}

static void _unset_acked_sync_flag() {
  g_comm_protocol_business.acked = false;
}

static void _set_acked_sync_flag() {
  g_comm_protocol_business.acked = true;
}

static uni_bool _is_acked_packet(char *protocol_buffer) {
  CommProtocolPacket *protocol_packet = (CommProtocolPacket *)protocol_buffer;
  return (protocol_packet->type == 0 &&
          protocol_packet->cmd == 0 &&
          protocol_packet->sequence == _current_sequence_get() &&
          protocol_packet->payload_len == 0);
}

static int _do_packet_attribute(CommAttribute *attribute) {
  /* acked process */
  int timeout;
  if (NULL == attribute || !attribute->need_acked) {
    return 0;
  }
  /* WAIT_ACK_TIMEOUT_MSEC_MAX ~ WAIT_ACK_TIMEOUT_MSEC_MIN 之间有效 */
  timeout = uni_min(attribute->timeout_msec, WAIT_ACK_TIMEOUT_MSEC_MAX);
  timeout = uni_max(timeout, WAIT_ACK_TIMEOUT_MSEC_MIN);
  //TODO stupid timeout, use select perf???
  while (timeout > 0) {
    timeout -= 5;
    usleep(5 * 1000);
    if (g_comm_protocol_business.acked) {
      break;
    }
  }
  if (!g_comm_protocol_business.acked) {
    LOGW(UART_COMM_TAG, "wait uart ack timeout");
  }
  return g_comm_protocol_business.acked ? 0 : E_UNI_COMM_PAYLOAD_ACK_TIMEOUT;
}

static CommProtocolPacket* _packet_alloc(int payload_len) {
  return (CommProtocolPacket *)uni_malloc(sizeof(CommProtocolPacket) +
                                          payload_len);
}

static void _packet_free(CommProtocolPacket *packet) {
  uni_free(packet);
}

static int _write_uart(CommProtocolPacket *packet, CommAttribute *attribute) {
  int ret = 0;
  if (NULL != g_comm_protocol_business.on_write) {
    /* sync uart write, we use mutex lock */
    pthread_mutex_lock(&g_comm_protocol_business.mutex);
    _unset_acked_sync_flag();
    g_comm_protocol_business.on_write((char *)packet,
                                       (int)_packet_len_get(packet));
    ret = _do_packet_attribute(attribute);
    pthread_mutex_unlock(&g_comm_protocol_business.mutex);
  }
  return ret;
}

static void _assmeble_packet(CommType type,
                             CommCmd cmd,
                             char *payload,
                             CommPayloadLen payload_len,
                             CommProtocolPacket *packet) {
  _sync_set(packet);
  _sequence_set(packet);
  _product_type_set(packet, type);
  _cmd_set(packet, cmd);
  _payload_set(packet, payload, payload_len);
  _payload_len_set(packet, payload_len);
  _checksum_calc(packet);
}

static uni_bool _is_protocol_buffer_overflow(int length) {
  return length >= PROTOCOL_BUF_SUPPORT_MAX_SIZE;
}

int CommProtocolPacketAssembleAndSend(CommType type, CommCmd cmd,
                                      char *payload,
                                      CommPayloadLen payload_len,
                                      CommAttribute *attribute) {
  int ret = 0;
  if (_is_protocol_buffer_overflow(sizeof(CommProtocolPacket) +
                                          payload_len)) {
    return E_UNI_COMM_PAYLOAD_TOO_LONG;
  }
  CommProtocolPacket *packet = _packet_alloc(payload_len);
  if (NULL == packet) {
    return E_UNI_COMM_ALLOC_FAILED;
  }
  _assmeble_packet(type, cmd, payload, payload_len, packet);
   ret = _write_uart(packet, attribute);
  _packet_free(packet);
  return ret;
}

static CommPacket* _packet_disassemble(char *buf) {
  CommPacket *packet;
  CommProtocolPacket *protocol_packet = (CommProtocolPacket *)buf;
  if (!_checksum_valid(protocol_packet)) {
    LOGE(UART_COMM_TAG, "checksum failed");
    return NULL;
  }
  packet = uni_malloc(sizeof(CommPacket) + _payload_len_get(protocol_packet));
  if (NULL == packet) {
    LOGE(UART_COMM_TAG, "alloc memory failed");
    return NULL;
  }
  packet->cmd = protocol_packet->cmd;
  packet->payload_len = _payload_len_get(protocol_packet);
  memcpy(packet->payload, _payload_get(protocol_packet),
         _payload_len_get(protocol_packet));
  return packet;
}

static void _enlarge_protocol_buffer(char **orginal, int *orginal_len) {
  char *p;
  int new_length = *orginal_len * 2;
  p = uni_malloc(new_length);
  memcpy(p, *orginal, *orginal_len);
  uni_free(*orginal);
  *orginal = p;
  *orginal_len = new_length;
}

/* small heap memory stays alway, only garbage collection big bins*/
static void _try_garbage_collection_protocol_buffer(char **buffer,
                                                    int *length) {
  if (*length >= PROTOCOL_BUF_GC_TRIGGER_SIZE) {
    uni_free(*buffer);
    *buffer = NULL;
    *length = DEFAULT_PROTOCOL_BUF_SIZE;
    LOGT(UART_COMM_TAG, "free buffer=%p, len=%d", *buffer, *length);
  }
}

static void _reset_protocol_buffer_status(int *index, int *length) {
  *index = 0;
  *length = 0;
}

static void _protocol_buffer_alloc(char **buffer, int *length, int index) {
  if (NULL == *buffer) {
    *buffer = uni_malloc(*length);
    LOGT(UART_COMM_TAG, "init buffer=%p, len=%d", *buffer, *length);
    return;
  }
  if (*length <= index) {
    _enlarge_protocol_buffer(buffer, length);
    LOGT(UART_COMM_TAG, "protocol buffer enlarge. p=%p, new len=%d",
         *buffer, *length);
    return;
  }
}

static void _one_protocol_frame_process(char *protocol_buffer) {
  /* when application not register hook, ignore all*/
  if (NULL == g_comm_protocol_business.on_recv_frame) {
    LOGW(UART_COMM_TAG, "donot register recv_frame hook");
    return;
  }
  /* ack frame donnot notify application, ignore it now */
  if (_is_acked_packet(protocol_buffer)) {
    LOGT(UART_COMM_TAG, "recv ack frame");
    _set_acked_sync_flag();
    return;
  }
  /* disassemble protocol buffer */
  CommPacket* packet = _packet_disassemble(protocol_buffer);
  if (NULL == packet) {
    LOGW(UART_COMM_TAG, "disassemble packet failed");
    return;
  }
  /* notify application when not ack frame */
  g_comm_protocol_business.on_recv_frame(packet);
  uni_free(packet);
}

static long _get_clock_time_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    return (ts.tv_sec * 1000L + (ts.tv_nsec / 1000000));
  }
  return 0;
}

static uni_bool _bytes_coming_speed_too_slow() {
  static long last_byte_coming_timestamp = 0;
  long now = _get_clock_time_ms();
  uni_bool timeout = false;
  if (now - last_byte_coming_timestamp > 100 &&
      now > last_byte_coming_timestamp) {
    timeout = true;
    LOGT(UART_COMM_TAG, "[%u->%u]", last_byte_coming_timestamp, now);
  }
  last_byte_coming_timestamp = now;
  return timeout;
}

static void _protocol_buffer_generate_byte_by_byte(char recv_c) {
  static int index = 0;
  static int length = 0;
  static int protocol_buffer_length = DEFAULT_PROTOCOL_BUF_SIZE;
  static char *protocol_buffer = NULL;
  /* check timestamp to reset status when physical error */
  if (index != 0 && _bytes_coming_speed_too_slow()) {
    LOGT(UART_COMM_TAG, "reset protocol buffer automatically[%d]", index);
    _reset_protocol_buffer_status(&index, &length);
    _try_garbage_collection_protocol_buffer(&protocol_buffer,
                                            &protocol_buffer_length);
  }
  /* protect heap use, cannot alloc large than 8K now */
  if (_is_protocol_buffer_overflow(protocol_buffer_length)) {
    /* drop remain bytes of this frame*/
    if (length > 1) {
      length--;
      return;
    }
    _reset_protocol_buffer_status(&index, &length);
    _try_garbage_collection_protocol_buffer(&protocol_buffer,
                                            &protocol_buffer_length);
    LOGW(UART_COMM_TAG, "recv invalid frame, payload too long");
    return;
  }
  _protocol_buffer_alloc(&protocol_buffer, &protocol_buffer_length, index);
  /* get frame header sync byte */
  if (LAYOUT_SYNC_IDX == index) {
    if (UNI_COMM_SYNC_VALUE == (unsigned char)recv_c) {
      protocol_buffer[index++] = recv_c;
    }
    return;
  }
  /* get payload length (low 8 bit)*/
  if (LAYOUT_PAYLOAD_LEN_LOW_IDX == index) {
    length = recv_c;
    LOGT(UART_COMM_TAG, "len low=%d", length);
  }
  /* get payload length (high 8 bit)*/
  if (LAYOUT_PAYLOAD_LEN_HIGH_IDX == index) {
    length += (((unsigned short)recv_c) << 8);
    LOGT(UART_COMM_TAG, "length=%d", length);
  }
  /* set protocol header */
  if (index < sizeof(CommProtocolPacket)) {
    protocol_buffer[index++] = recv_c;
    goto L_END;
  }
  /* set protocol payload */
  if (sizeof(CommProtocolPacket) <= index && 0 < length) {
    protocol_buffer[index++] = recv_c;
    length--;
  }
L_END:
  /* callback protocol buffer */
  if (sizeof(CommProtocolPacket) <= index && 0 == length) {
    _one_protocol_frame_process(protocol_buffer);
    _reset_protocol_buffer_status(&index, &length);
    _try_garbage_collection_protocol_buffer(&protocol_buffer,
                                            &protocol_buffer_length);
  }
}

void CommProtocolReceiveUartData(char *buf, int len) {
  for (int i = 0; i < len; i++) {
    _protocol_buffer_generate_byte_by_byte(buf[i]);
  }
}

static void _register_packet_receive_handler(RecvCommPacketHandler handler) {
  g_comm_protocol_business.on_recv_frame = handler;
}

static void _unregister_packet_receive_handler() {
  g_comm_protocol_business.on_recv_frame = NULL;
}

static void _protocol_business_init() {
  memset(&g_comm_protocol_business, 0, sizeof(g_comm_protocol_business));
  pthread_mutex_init(&g_comm_protocol_business.mutex, NULL);
}

static void _protocol_business_final() {
  pthread_mutex_destroy(&g_comm_protocol_business.mutex);
  memset(&g_comm_protocol_business, 0, sizeof(g_comm_protocol_business));
}

int CommProtocolInit(CommWriteHandler write_handler,
                     RecvCommPacketHandler recv_handler) {
  _protocol_business_init();
  _register_write_handler(write_handler);
  _register_packet_receive_handler(recv_handler);
  return 0;
}

void CommProtocolFinal() {
  _unregister_packet_receive_handler();
  _unregister_write_handler();
  _protocol_business_final();
}
