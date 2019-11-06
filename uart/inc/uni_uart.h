#ifndef _UTILS_UNI_UART_H_
#define _UTILS_UNI_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <termios.h>

#define UNI_UART_DEVICE_NAME_MAX  (64)

typedef int (*UniUartFrameData)(char *buf, int len);

typedef struct {
  char    device[UNI_UART_DEVICE_NAME_MAX];
  speed_t speed;
} UniUartConfig;

int UniUartRegisterRecvFrameHandler(UniUartFrameData handler);
int UniUartUnregisterRecvFrameHandler();
int UniUartInitialize(UniUartConfig *config);
int UniUartFinalize();
int UniUartWrite(char *buf, int len);

#ifdef __cplusplus
}
#endif
#endif

