#ifndef _UTILS_UNI_COMMUNICATION_H_
#define _UTILS_UNI_COMMUNICATION_H_

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
