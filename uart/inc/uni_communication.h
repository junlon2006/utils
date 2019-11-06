#ifndef _UTILS_UNI_COMMUNICATION_H_
#define _UTILS_UNI_COMMUNICATION_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short      UniCommCmd;
typedef unsigned short      UniCommPayloadLen;
typedef unsigned short      UniCommType;
typedef int                 (*UniCommWriteHandler)(char *buf, int len);

#define UNI_COMM_SYNC_VALUE (0xFF)
#define UNI_COMM_TYPE_BASE  (0)
#define UNI_DEMO            (UNI_COMM_TYPE_BASE + 1)
#define PACKED              __attribute__ ((packed))

typedef struct {
  UniCommCmd        cmd;
  UniCommPayloadLen payload_len;
  char              payload[0];
} PACKED UniCommPacket;

int            UniCommProtocolRegisterWriteHandler(UniCommWriteHandler handler);
int            UniCommProtocolPacketAssembleAndSend(UniCommType type, UniCommCmd cmd,
                                                    char *payload, UniCommPayloadLen payload_len);
UniCommPacket *UniCommProtocolPacketDisassemble(char *buf, int len);
int            UniCommPacketFree(UniCommPacket *packet);

#ifdef __cplusplus
}
#endif
#endif
